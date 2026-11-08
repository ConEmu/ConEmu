# Explicit shell integration through ConEmuC IPC. Compatible with PowerShell 5.1.
$script:ShellIntegration = $null

function Get-ConEmuHelper {
    $name = if ([Environment]::Is64BitOperatingSystem) { 'ConEmuC64.exe' } else { 'ConEmuC.exe' }
    $helper = Join-Path (Split-Path $PSScriptRoot -Parent) $name
    if (-not (Test-Path -LiteralPath $helper)) {
        $helper = Join-Path (Split-Path $PSScriptRoot -Parent) 'ConEmuC.exe'
    }
    if (-not (Test-Path -LiteralPath $helper)) {
        throw 'ConEmuC was not found next to the Scripts directory.'
    }
    return $helper
}

function ConvertTo-ConEmuString([string] $Value) {
    # GuiMacro verbatim strings escape quotation marks by doubling them.
    return '@"' + $Value.Replace('"', '""') + '"'
}

function Invoke-ConEmuHelper([string[]] $HelperArguments) {
    # Quote each native argument explicitly, including trailing backslashes.
    # PowerShell 5.1 and PowerShell 7 use different native argument marshalling.
    $quoted = foreach ($argument in $HelperArguments) {
        $escaped = [regex]::Replace($argument, '(\\*)"', '$1$1\"')
        '"' + [regex]::Replace($escaped, '(\\+)$', '$1$1') + '"'
    }
    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = Get-ConEmuHelper
    # /GuiMacro consumes the raw command-line remainder, not a quoted argv item.
    if ($HelperArguments[0] -eq '/GuiMacro:T-2') {
        $start.Arguments = '/GuiMacro:T-2 ' + $HelperArguments[1]
    }
    else {
        $start.Arguments = $quoted -join ' '
    }
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $process = [System.Diagnostics.Process]::Start($start)
    try {
        $result = $process.StandardOutput.ReadToEnd()
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) {
            throw "ConEmuC failed ($($process.ExitCode)). $result"
        }
        return $result.TrimEnd()
    }
    finally {
        $process.Dispose()
    }
}

function Invoke-ConEmuMacro {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string] $Macro)

    # Address the calling console even after another tab becomes active.
    Invoke-ConEmuHelper @('/GuiMacro:T-2', $Macro)
}

function New-ConEmuConsole {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string] $Command,
        [string] $Arguments = '',
        [string] $Directory = $PWD.ProviderPath,
        [ValidatePattern('^new_console(:[A-Za-z0-9]+)?$')]
        [string] $Options = 'new_console'
    )

    $values = @($Options, $Command, $Arguments, $Directory) |
        ForEach-Object { ConvertTo-ConEmuString $_ }
    Invoke-ConEmuMacro ('Shell(' + ($values -join ',') + ')')
}

function Set-ConEmuTabTitle {
    [CmdletBinding()]
    param([AllowEmptyString()][Parameter(Mandatory = $true)][string] $Title)

    Invoke-ConEmuMacro ('Rename(0,' + (ConvertTo-ConEmuString $Title) + ')')
}

function Set-ConEmuProgress {
    [CmdletBinding()]
    param(
        [ValidateSet('None', 'Running', 'Error', 'Indeterminate', 'Paused')]
        [string] $State = 'Running',
        [ValidateRange(0, 100)][int] $Value = 0
    )

    $states = @{ None = 0; Running = 1; Error = 2; Indeterminate = 3; Paused = 4 }
    Invoke-ConEmuMacro ('Progress(' + $states[$State] + ',' + $Value + ')')
}

function Sync-ConEmuDirectory {
    [CmdletBinding()]
    param()

    if ($PWD.Provider.Name -eq 'FileSystem') {
        Invoke-ConEmuHelper @('/StoreCWD', $PWD.ProviderPath)
    }
}

function Start-ConEmuPty {
    [CmdletBinding()]
    param([Parameter(Mandatory = $true)][string] $CommandLine)

    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = Get-ConEmuHelper
    $start.Arguments = '/PTY ' + $CommandLine
    $start.UseShellExecute = $false
    $process = [System.Diagnostics.Process]::Start($start)
    try {
        $process.WaitForExit()
        $global:LASTEXITCODE = $process.ExitCode
    }
    finally {
        $process.Dispose()
    }
}

Export-ModuleMember -Function Invoke-ConEmuMacro, New-ConEmuConsole,
    Set-ConEmuTabTitle, Set-ConEmuProgress, Sync-ConEmuDirectory, Start-ConEmuPty
function Enable-ConEmuShellIntegration {
    [CmdletBinding()]
    param([switch] $UpdateTitle)
    if ($script:ShellIntegration) { return }
    if ([Console]::IsOutputRedirected) { throw 'Shell integration requires an interactive console.' }
    $state = [pscustomobject]@{
        Prompt = (Get-Item Function:prompt -ErrorAction SilentlyContinue).ScriptBlock
        HistoryHandler = $null
        HistoryInstalled = $false
        UpdateTitle = [bool]$UpdateTitle
        CommandRunning = $false
    }
    $script:ShellIntegration = $state
    $esc = [char]27
    $prompt = {
        $ok = $?
        $exitCode = $global:LASTEXITCODE
        if ($state.CommandRunning) {
            [Console]::Write("${esc}]133;D;$(if ($ok) { 0 } else { 1 })${esc}\")
            $state.CommandRunning = $false
        }
        if ($PWD.Provider.Name -eq 'FileSystem') {
            $uri = ([Uri]($PWD.ProviderPath.TrimEnd('\') + '\')).AbsoluteUri
            [Console]::Write("${esc}]7;$uri${esc}\")
        }
        [Console]::Write("${esc}]9;4;0${esc}\${esc}]133;A${esc}\")
        if ($state.UpdateTitle) {
            $title = $PWD.Path -replace '[\x00-\x1f\x7f]', ''
            [Console]::Write("${esc}]2;$title${esc}\")
        }
        if ($state.Prompt) { & $state.Prompt } else { "PS $($PWD)> " }
        [Console]::Write("${esc}]133;B${esc}\")
        $global:LASTEXITCODE = $exitCode
    }.GetNewClosure()
    Set-Item Function:global:prompt $prompt
    if (Get-Command Get-PSReadLineOption -ErrorAction SilentlyContinue) {
        $state.HistoryHandler = (Get-PSReadLineOption).AddToHistoryHandler
        $handler = {
            param([string]$line)
            $state.CommandRunning = $true
            [Console]::Write("${esc}]133;C${esc}\${esc}]9;4;3${esc}\")
            if ($state.HistoryHandler) { return $state.HistoryHandler.Invoke($line) }
            return $true
        }.GetNewClosure()
        Set-PSReadLineOption -AddToHistoryHandler $handler
        $state.HistoryInstalled = $true
    }
}

function Disable-ConEmuShellIntegration {
    [CmdletBinding()]
    param()
    $state = $script:ShellIntegration
    if (!$state) { return }
    if ($state.Prompt) { Set-Item Function:global:prompt $state.Prompt }
    else { Remove-Item Function:global:prompt -ErrorAction SilentlyContinue }
    if ($state.HistoryInstalled) { Set-PSReadLineOption -AddToHistoryHandler $state.HistoryHandler }
    [Console]::Write("$([char]27)]9;4;0$([char]27)\")
    $script:ShellIntegration = $null
}

Export-ModuleMember -Function Enable-ConEmuShellIntegration, Disable-ConEmuShellIntegration
function Start-ConEmuIntegratedPowerShell {
    [CmdletBinding()]
    param([string] $Executable = '', [switch] $UpdateTitle)
    if (!$Executable) {
        $Executable = Join-Path $PSHOME $(if ($PSVersionTable.PSEdition -eq 'Core') { 'pwsh.exe' } else { 'powershell.exe' })
    }
    $module = (Join-Path $PSScriptRoot 'ConEmu.NoHooks.psm1').Replace("'", "''")
    $command = "Import-Module '$module'; Enable-ConEmuShellIntegration" + $(if ($UpdateTitle) { ' -UpdateTitle' } else { '' })
    $encoded = [Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($command))
    $quoted = '"' + [regex]::Replace($Executable, '(\\*)"', '$1$1\"') + '"'
    Start-ConEmuPty -CommandLine ($quoted + ' -NoLogo -NoExit -EncodedCommand ' + $encoded)
}
Export-ModuleMember -Function Start-ConEmuIntegratedPowerShell

$ExecutionContext.SessionState.Module.OnRemove = {
    if ($script:ShellIntegration) { Disable-ConEmuShellIntegration }
}
