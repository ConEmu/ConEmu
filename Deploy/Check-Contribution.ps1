param(
    [string]$Base,
    [string]$PremakePath
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot

function Invoke-Git {
    & git -c core.quotePath=false @args
    if ($LASTEXITCODE -ne 0) { throw "git $args failed ($LASTEXITCODE)." }
}

function Test-BlobBom([string]$Revision, [string]$Path) {
    $object = Invoke-Git rev-parse --verify "${Revision}:$Path"
    $options = New-Object System.Diagnostics.ProcessStartInfo
    $options.FileName = 'git'
    $options.Arguments = "cat-file blob $object"
    $options.WorkingDirectory = $root
    $options.UseShellExecute = $false
    $options.RedirectStandardOutput = $true
    $process = [System.Diagnostics.Process]::Start($options)
    try {
        $stream = $process.StandardOutput.BaseStream
        $first = $stream.ReadByte()
        $second = $stream.ReadByte()
        $third = $stream.ReadByte()
        $stream.CopyTo([System.IO.Stream]::Null)
        $process.WaitForExit()
        if ($process.ExitCode -ne 0) { throw "Cannot read $Revision $Path." }
        return $first -eq 239 -and $second -eq 187 -and $third -eq 191
    } finally {
        $process.Dispose()
    }
}

Push-Location -LiteralPath $root
$temp = $null
try {
    if ($Base) {
        $mergeBase = Invoke-Git merge-base $Base HEAD
        Invoke-Git diff --check $mergeBase HEAD
        $changes = @(Invoke-Git diff --name-status --no-renames $mergeBase HEAD)
        $hasProviderChanges = $false
        $hasProductChanges = $false
        foreach ($entry in $changes) {
            $status, $path = $entry -split "`t", 2
            if ($path -match '^(\.github/workflows/|appveyor\.yml$|azure-pipelines\.yml$|Deploy/Check-Contribution\.ps1$)') {
                $hasProviderChanges = $true
            }
            if ($path -match '^(src/|Release/ConEmu/)' -and $path -notmatch '\.(vcxproj(\.filters)?|sln|md|txt)$') {
                $hasProductChanges = $true
            }
            if ($status -eq 'M' -and $path -match '\.(c|cpp|h|hpp|rc|rc2|vcxproj(\.filters)?|sln)$') {
                if ((Test-BlobBom $mergeBase $path) -and -not (Test-BlobBom HEAD $path)) {
                    throw "Restore the UTF-8 BOM in $path."
                }
                git diff -w --ignore-blank-lines --quiet $mergeBase HEAD -- $path
                if ($LASTEXITCODE -eq 0) { throw "Drop whitespace-only changes in $path." }
                if ($LASTEXITCODE -ne 1) { throw "Could not compare whitespace in $path." }
            }
            if ($status -eq 'A' -and $path -match '^src/ConEmu/.*\.(c|cpp|h|hpp)$' -and
                -not (Test-BlobBom HEAD $path)) {
                throw "Write $path as UTF-8 with a BOM."
            }
        }
        if ($hasProviderChanges -and $hasProductChanges) {
            throw 'Submit CI provider changes separately from product source changes.'
        }
    }

    $projects = @('*.vcxproj', '*.vcxproj.filters', '*.sln')
    Invoke-Git diff --exit-code HEAD -- @projects
    if (-not $PremakePath) {
        $temp = Join-Path ([System.IO.Path]::GetTempPath()) ('conemu-premake-' + [guid]::NewGuid())
        New-Item -ItemType Directory -Path $temp | Out-Null
        $archive = Join-Path $temp 'premake.zip'
        [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/premake/premake-core/releases/download/v5.0.0-alpha15/premake-5.0.0-alpha15-windows.zip' -OutFile $archive
        if ((Get-FileHash -Algorithm SHA256 -LiteralPath $archive).Hash -ne 'A8B9FC87EC06B8FA52587F62BE2574BF151C5FACADBAD99DB31B7D4CE6EF1EAB') {
            throw 'Premake archive checksum mismatch.'
        }
        Expand-Archive -LiteralPath $archive -DestinationPath $temp
        $PremakePath = Join-Path $temp 'premake5.exe'
    }
    & $PremakePath vs2019
    if ($LASTEXITCODE -ne 0) { throw 'Premake generation failed.' }

    # Ignore the equivalent header spelling that differs in the VS2019 solution.
    $solution = Join-Path $root 'src/CE.sln'
    $content = [IO.File]::ReadAllText($solution).Replace('# Visual Studio Version 16', '# Visual Studio 16')
    [IO.File]::WriteAllText($solution, $content, [Text.UTF8Encoding]::new($true))
    Invoke-Git diff --exit-code HEAD -- @projects
    Write-Output 'Contribution and VS2019 project generation checks passed.'
} finally {
    Pop-Location
    if ($temp) { Remove-Item -LiteralPath $temp -Recurse -Force }
}
