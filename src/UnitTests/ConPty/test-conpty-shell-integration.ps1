param([string]$Root,[string]$Report)
$ErrorActionPreference='Stop'
$lines=New-Object 'Collections.Generic.List[string]'
function Check([bool]$ok,[string]$label){if(!$ok){throw $label};$lines.Add('PASS '+$label)}
try {
 function global:prompt { 'ORIGINAL_PROMPT' }
 $original=(Get-Item Function:prompt).ScriptBlock
 Import-Module PSReadLine -ErrorAction SilentlyContinue
 $originalHandler=if(Get-Command Get-PSReadLineOption -ErrorAction SilentlyContinue){(Get-PSReadLineOption).AddToHistoryHandler}else{$null}
 $module=Import-Module (Join-Path $Root 'Release/ConEmu/Scripts/ConEmu.NoHooks.psm1') -Force -PassThru
 $global:LASTEXITCODE=37
 Enable-ConEmuShellIntegration -UpdateTitle
 $text=prompt
 Check (($text -join '') -eq 'ORIGINAL_PROMPT') 'Original prompt output preserved'
 Check ($global:LASTEXITCODE -eq 37) 'Native exit code survives prompt integration'
 if(Get-Command Get-PSReadLineOption -ErrorAction SilentlyContinue){
   $handler=(Get-PSReadLineOption).AddToHistoryHandler
   $null=$handler.Invoke('Write-Output test')
   Check (& $module {$script:ShellIntegration.CommandRunning}) 'PSReadLine command-start handler installed'
   $null=prompt
   Check (!(& $module {$script:ShellIntegration.CommandRunning})) 'Prompt emits completion and clears running state'
 }
 $once=(Get-Item Function:prompt).ScriptBlock.ToString()
 Enable-ConEmuShellIntegration
 Check ((Get-Item Function:prompt).ScriptBlock.ToString() -eq $once) 'Repeated enable does not nest prompt wrappers'
 Disable-ConEmuShellIntegration
 Check ((Get-Item Function:prompt).ScriptBlock.ToString() -eq $original.ToString()) 'Disable restores original prompt'
 if(Get-Command Get-PSReadLineOption -ErrorAction SilentlyContinue){Check ((Get-PSReadLineOption).AddToHistoryHandler -eq $originalHandler) 'Disable restores original PSReadLine handler'}
 Enable-ConEmuShellIntegration
 Remove-Module $module
 Check ((Get-Item Function:prompt).ScriptBlock.ToString() -eq $original.ToString()) 'Module removal restores prompt'
 $lines | Set-Content -LiteralPath $Report
 exit 0
} catch { $lines.Add('FAIL '+$_);$lines | Set-Content -LiteralPath $Report;exit 1 }
