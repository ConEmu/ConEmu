# Remote test VM setup for ConEmu (Windows 11 Pro x64)

Goal: a VM `win11en` where `ConEmu64.exe` can be launched and debugged both
manually (RDP + VS 2019 Remote Debugger) and by a coding agent (SSH +
scheduled tasks + `cdb`).

The reference setup runs the VM on a NAS server, but everything inside the
guest applies to any hypervisor.

> **Warning: for a dedicated test VM in a trusted LAN only.** Do not apply
> this to your workstation. The setup deliberately weakens security:
> Autologon, `LocalAccountTokenFilterPolicy=1` (full admin token over the
> network), Defender exclusions, a passwordless SSH key, and
> `C:\agent\run-args.txt`, which runs any command written to it.

**Names used below are examples**; replace them consistently if yours
differ:

| Name | Meaning |
|---|---|
| `win11en` | VM host name |
| `smbuser` | local admin user on the VM (SMB, RDP, SSH, Autologon) |
| `C:\ConEmu` | build output folder on the VM, shared as `ConEmu` |
| `Z:` | that share mapped on the developer host |
| `C:\agent` | launcher scripts for the agent (not shared) |
| `C:\dumps` | crash dumps of `ConEmu64.exe` |
| `192.168.x.y` | VM IPv4 address reserved in the router |

Block 1 takes most of them from variables at its top; other blocks have
them hardcoded.

All PowerShell blocks are run **on the VM in an elevated PowerShell**, unless
stated otherwise. Blocks are safe to re-run: they are either idempotent or
overwrite settings.

---

## 0. Manual steps (before and between blocks)

1. **virtio drivers and QEMU guest agent** (QEMU/KVM hosts only). Download
   the stable `virtio-win.iso` from fedorapeople.org, attach it to the VM as
   a CD-ROM and run `virtio-win-guest-tools.exe`. It covers the
   `VEN_1AF4&DEV_1045` (balloon) and `VEN_1AF4&DEV_1043` (vioserial) devices
   and installs `qemu-ga`. Check: `Get-Service QEMU-GA`.
2. **VM graphics adapter**: `vga` or `virtio` (the latter with the
   `viogpudo` driver from virtio-win). Without GPU passthrough, a hardware
   USB-C KVM (e.g. NanoKVM) cannot capture the VM screen; use RDP/VNC
   instead.
3. After block 1, **fully sign out** (Sign out, not Disconnect) and sign in
   again: Administrators group membership is applied only at a new logon.
4. **Autologon** (Sysinternals):
   `Autologon64.exe smbuser win11en <password> /accepteula`, or use its GUI.
5. **Debugging Tools for Windows** (provides `cdb.exe`, required by
   block 4). Needed on the VM and on the developer host (for
   mcp-windbg).
   - Download `winsdksetup.exe` from
     <https://learn.microsoft.com/en-us/windows/apps/windows-sdk/downloads>.
   - Install only the debuggers: select "Debugging Tools for Windows" in
     the GUI, or run:

     ```
     winsdksetup.exe /features OptionId.WindowsDesktopDebuggers ^
       /quiet /norestart
     ```

   - Check (must print `True`):

     ```powershell
     $dbg = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64"
     Test-Path "$dbg\cdb.exe"
     ```
6. **VS 2019 Remote Tools x64** (`VS_RemoteTools_2019_x64.exe`). Use the
   Remote Tools matching your Visual Studio version.
7. To run these blocks saved as a `.ps1` file, allow local scripts once:
   `Set-ExecutionPolicy RemoteSigned -Scope LocalMachine -Force`.
   If a copied script is still blocked: `Unblock-File <path>`.
8. When done, take a **VM snapshot**.

---

## 1. User, network, share, RDP, power

```powershell
# ===== Parameters =====
$User      = "smbuser"
$ShareName = "ConEmu"
$SharePath = "C:\ConEmu"
$AgentPath = "C:\agent"
$DumpPath  = "C:\dumps"

# ===== Local admin user =====
if (-not (Get-LocalUser -Name $User -ErrorAction SilentlyContinue)) {
    $pw = Read-Host -AsSecureString "Password for $User"
    New-LocalUser -Name $User -Password $pw -PasswordNeverExpires |
        Out-Null
}
Add-LocalGroupMember -Group "Administrators" -Member $User `
    -ErrorAction SilentlyContinue

# UAC: keep it enabled, plain Yes/No prompt for admins (Windows default)
$pol = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\System"
Set-ItemProperty $pol -Name ConsentPromptBehaviorAdmin -Value 5
# Allow full admin token for local accounts over the network (C$, etc.)
New-ItemProperty $pol -Name LocalAccountTokenFilterPolicy -Value 1 `
    -PropertyType DWord -Force | Out-Null

# ===== Network: Private profile + discovery + file sharing =====
Get-NetConnectionProfile | Set-NetConnectionProfile -NetworkCategory Private
Enable-NetFirewallRule -DisplayGroup "Network Discovery"
# File and Printer Sharing also includes inbound ICMP (ping)
Enable-NetFirewallRule -DisplayGroup "File and Printer Sharing"
foreach ($svc in "fdPHost", "FDResPub", "SSDPSRV", "upnphost") {
    Set-Service $svc -StartupType Automatic
    Start-Service $svc -ErrorAction SilentlyContinue
}

# ===== Shared folder C:\ConEmu =====
New-Item -ItemType Directory -Force -Path $SharePath, $AgentPath, $DumpPath |
    Out-Null
if (-not (Get-SmbShare -Name $ShareName -ErrorAction SilentlyContinue)) {
    New-SmbShare -Name $ShareName -Path $SharePath `
        -FullAccess "$env:COMPUTERNAME\$User" | Out-Null
}
icacls $SharePath /grant "${User}:(OI)(CI)F" | Out-Null

# ===== RDP =====
$ts = "HKLM:\System\CurrentControlSet\Control\Terminal Server"
Set-ItemProperty $ts -Name fDenyTSConnections -Value 0
Enable-NetFirewallRule -DisplayGroup "Remote Desktop"

# ===== Power: never sleep, display never off, no hibernation =====
powercfg /change standby-timeout-ac 0
powercfg /change monitor-timeout-ac 0
powercfg /hibernate off

# ===== Windows Update: no auto-reboot while a user is logged on =====
$wu = "HKLM:\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU"
New-Item $wu -Force | Out-Null
Set-ItemProperty $wu -Name NoAutoRebootWithLoggedOnUsers -Value 1

# ===== Defender: ConEmu hooks (ConEmuHk) often trigger false positives =====
Add-MpPreference -ExclusionPath $SharePath, $AgentPath

# ===== Crash dumps for the agent (WER LocalDumps) =====
$wer = "HKLM:\SOFTWARE\Microsoft\Windows\Windows Error Reporting"
$ld  = Join-Path $wer "LocalDumps\ConEmu64.exe"
New-Item $ld -Force | Out-Null
New-ItemProperty $ld -Name DumpFolder -Value $DumpPath `
    -PropertyType ExpandString -Force | Out-Null
# DumpType 2 = full dump
New-ItemProperty $ld -Name DumpType -Value 2 `
    -PropertyType DWord -Force | Out-Null
New-ItemProperty $ld -Name DumpCount -Value 10 `
    -PropertyType DWord -Force | Out-Null
```

After this block: **Sign out and sign in as `smbuser`** (step 0.3). Check in
a regular (non-elevated) PowerShell:

```
whoami /groups | findstr S-1-5-32-544
```

It must print a line with `Group used for deny only`.

Run the remaining blocks as `smbuser`.

```powershell
# Explorer: show file extensions and hidden files (current user)
$adv = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced"
Set-ItemProperty $adv -Name HideFileExt -Value 0
Set-ItemProperty $adv -Name Hidden -Value 1
```

---

## 2. OpenSSH Server and key-based login

```powershell
# Install OpenSSH Server (downloads from Windows Update, takes a while)
Add-WindowsCapability -Online -Name OpenSSH.Server~~~~0.0.1.0
Set-Service sshd -StartupType Automatic
Start-Service sshd

# PowerShell as the default shell for SSH sessions
$ps = "C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe"
New-ItemProperty -Path "HKLM:\SOFTWARE\OpenSSH" -Name DefaultShell `
    -Value $ps -PropertyType String -Force | Out-Null

# Firewall rule (usually created by the installer for the Private profile
# only; make sure it exists)
$rule = "OpenSSH-Server-In-TCP"
if (-not (Get-NetFirewallRule -Name $rule -ErrorAction SilentlyContinue)) {
    New-NetFirewallRule -Name $rule -DisplayName "OpenSSH Server" `
        -Direction Inbound -Protocol TCP -LocalPort 22 `
        -RemoteAddress LocalSubnet -Action Allow | Out-Null
}

# Public key: for members of Administrators OpenSSH reads a shared file,
# NOT ~\.ssh\authorized_keys
$keys = "C:\ProgramData\ssh\administrators_authorized_keys"
$pub  = Read-Host "Paste the public key line (ssh-ed25519 AAAA... comment)"
$have = if (Test-Path $keys) { Get-Content $keys } else { @() }
if ($have -notcontains $pub) { Add-Content -Path $keys -Value $pub }
icacls $keys /inheritance:r /grant "Administrators:F" /grant "SYSTEM:F" |
    Out-Null
```

The trailing comment of the key (usually an email) does not matter.

If `Add-WindowsCapability` fails, install via Settings → System → Optional
features → View features → OpenSSH Server.

---

## 3. Debugging: VS 2019 Remote Debugger (for a human)

```powershell
# Autostart msvsmon in smbuser's interactive session (Startup folder, not
# a service: a service runs in session 0, so a launched GUI is invisible)
$vs      = "C:\Program Files\Microsoft Visual Studio 16.0"
$msvsmon = Join-Path $vs "Common7\IDE\Remote Debugger\x64\msvsmon.exe"
$startup = Join-Path $env:APPDATA "Microsoft\Windows\Start Menu\Programs"
$lnk     = Join-Path $startup "Startup\msvsmon.lnk"
$s = (New-Object -ComObject WScript.Shell).CreateShortcut($lnk)
$s.TargetPath = $msvsmon
$s.Arguments  = "/port 4024"
$s.Save()

# VS 2019 remote debugger port. Only 4024 is opened: enough for x64.
# To also debug 32-bit processes, use -LocalPort 4024,4025
Remove-NetFirewallRule -DisplayName "msvsmon 2019" `
    -ErrorAction SilentlyContinue
New-NetFirewallRule -DisplayName "msvsmon 2019" -Direction Inbound `
    -Protocol TCP -LocalPort 4024 -RemoteAddress LocalSubnet `
    -Action Allow | Out-Null
```

Authentication: Windows Authentication (default), no `/noauth`.

Default ports: VS 2019 uses 4024; VS 2022 and VS 2026 use 4026. Remote
Tools 2019 work only with VS 2019; Remote Tools 2026 work with VS 2022 and
later.

Connecting from VS 2019:

- **Attach:** Debug → Attach to Process → *Remote (Windows)* →
  `win11en:4024`, credentials `win11en\smbuser`.
- **Launch:** Project Properties → Debugging → *Remote Windows Debugger*:
  - Remote Command: `C:\ConEmu\ConEmu64.exe`
  - Remote Server Name: `win11en:4024`
  - Connection: *Remote with Windows authentication*

---

## 4. Launching and debugging by an agent

Roles:

- **Developer host**: sources, Visual Studio, the coding agent. Builds go
  to the VM through the share (`Z:` → `\\win11en\ConEmu` → `C:\ConEmu`).
- **VM (`win11en`)**: runs and debugs the built binaries.

How it works: the agent on the developer host calls
`ssh win11en C:\agent\start.ps1 <command line>`. The script writes the
command line to `C:\agent\run-args.txt` and starts a scheduled task. The
task runs in `smbuser`'s **interactive session**, so the window is
visible. Processes started directly from SSH or from a service run in a
non-interactive session.

`C:\agent` is deliberately outside the share: whoever writes
`run-args.txt` can run arbitrary commands, so it is changed only over SSH
(key-based login), and deploying builds to `C:\ConEmu` cannot break it.

```powershell
$AgentPath = "C:\agent"
$Cdb = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\cdb.exe"

# run.cmd: start the command line from run-args.txt, detached
@"
@echo off
rem Read the full command line from run-args.txt and start it detached
set /p CMDLINE=<$AgentPath\run-args.txt
start "" %CMDLINE%
"@ | Set-Content -Encoding ASCII "$AgentPath\run.cmd"

# debug.cmd: same, under a cdb debug server on TCP 5005
@"
@echo off
rem Start the command line from run-args.txt under cdb server (tcp:5005)
set /p CMDLINE=<$AgentPath\run-args.txt
start "" "$Cdb" -server tcp:port=5005 %CMDLINE%
"@ | Set-Content -Encoding ASCII "$AgentPath\debug.cmd"

# start.ps1: entry point for the agent over SSH.
# Usage: start.ps1 [-Dbg] <command line...>
@'
param([switch]$Dbg)
# Remaining (unbound) arguments form the command line to run
Set-Content -Encoding ASCII "C:\agent\run-args.txt" ($args -join " ")
if ($Dbg) { Start-ScheduledTask AgentDebug }
else      { Start-ScheduledTask AgentRun }
'@ | Set-Content -Encoding ASCII "$AgentPath\start.ps1"

# On-demand tasks in smbuser's interactive session; no password needed
$p = New-ScheduledTaskPrincipal -UserId "smbuser" `
    -LogonType Interactive -RunLevel Limited
$runAction   = New-ScheduledTaskAction -Execute "$AgentPath\run.cmd"
$debugAction = New-ScheduledTaskAction -Execute "$AgentPath\debug.cmd"
Register-ScheduledTask -TaskName AgentRun -Principal $p `
    -Action $runAction -Force | Out-Null
Register-ScheduledTask -TaskName AgentDebug -Principal $p `
    -Action $debugAction -Force | Out-Null

# cdb debug server port
Remove-NetFirewallRule -DisplayName "cdb server" `
    -ErrorAction SilentlyContinue
New-NetFirewallRule -DisplayName "cdb server" -Direction Inbound `
    -Protocol TCP -LocalPort 5005 -RemoteAddress LocalSubnet `
    -Action Allow | Out-Null
```

`-RunLevel Limited` starts ConEmu non-elevated. Testing elevated launch
requires a separate task with `-RunLevel Highest`. The tasks only work
while `smbuser` is logged on, which Autologon ensures.

Check on the VM:

```powershell
C:\agent\start.ps1 C:\ConEmu\ConEmu64.exe /basic
```

Agent commands (from the developer host):

- launch: `ssh win11en C:\agent\start.ps1 C:\ConEmu\ConEmu64.exe /basic`;
- debug: `ssh win11en C:\agent\start.ps1 -Dbg C:\ConEmu\ConEmu64.exe`,
  then mcp-windbg `open_cdb_remote` → `tcp:server=win11en,port=5005`;
- stop: `ssh win11en "Stop-Process -Name ConEmu64"`;
- crash dumps: `\\win11en\C$\dumps` (or `C:\dumps` over SSH).

---

## 5. Developer host (client)

**Stable address**: reserve an IPv4 address for the VM in the router's DHCP
and add it to `C:\Windows\System32\drivers\etc\hosts`:

```
192.168.x.y    win11en
```

Otherwise the name may resolve via mDNS to an IPv6 link-local address,
which makes RDP slow to connect.

**SSH config**: `%USERPROFILE%\.ssh\config`:

```
Host win11en
    HostName win11en
    User smbuser
    IdentityFile ~/.ssh/id_ed25519_vm
```

Check: `ssh win11en hostname`.

**Non-interactive SSH for the agent.** The coding agent cannot type a key
passphrase. Two options that avoid a system-wide `ssh-agent` service
(which would expose every loaded key, e.g. a GitHub key, to any process
of the user):

1. *Dedicated passwordless key for the VM only*, restricted on the VM side
   to the LAN. Generate it on the developer host:

   ```powershell
   # Press Enter twice at the passphrase prompt (empty passphrase)
   ssh-keygen -t ed25519 -C vm-agent `
       -f "$env:USERPROFILE\.ssh\id_ed25519_vm_agent"
   ```

   On the VM, prefix its line in `administrators_authorized_keys` with
   a source restriction:

   ```
   from="192.168.x.0/24" ssh-ed25519 AAAA... vm-agent
   ```

   Point `IdentityFile` in the SSH config to this key. The key grants
   access to the test VM only.

2. *Session-scoped agent* (Git Bash; keeps the passphrase-protected key).
   Start it right before the coding agent; the agent inherits
   `SSH_AUTH_SOCK`, and the keys disappear when it is killed:

   ```bash
   eval "$(ssh-agent -s)"
   ssh-add -t 8h ~/.ssh/id_ed25519_vm
   claude                     # or another coding agent
   ssh-agent -k
   ```

   This works with the Git for Windows `ssh`, not with the Windows
   OpenSSH client (`C:\Windows\System32\OpenSSH\ssh.exe`), which only
   talks to the Windows `ssh-agent` service.

**Network drive:**

```
net use Z: \\win11en\ConEmu /user:win11en\smbuser /persistent:yes
```

**RDP file** (`Win11en.rdp`): full screen on the monitor with ID 1 (list
IDs with `mstsc /l`):

```
screen mode id:i:2
use multimon:i:1
selectedmonitors:s:1
dynamic resolution:i:1
full address:s:win11en
```

`selectedmonitors` only works together with `use multimon:i:1`. Manual
alternative: open windowed on the target monitor and press
`Ctrl+Alt+Break`.

**Signing the `.rdp` file** (removes the "Unknown publisher" warning;
re-sign after any edit):

```powershell
$cert = New-SelfSignedCertificate -Subject "CN=Local RDP signing" `
    -Type CodeSigningCert -CertStoreLocation Cert:\CurrentUser\My
$tmp = "$env:TEMP\rdp.cer"
Export-Certificate -Cert $cert -FilePath $tmp | Out-Null
foreach ($store in "Root", "TrustedPublisher") {
    Import-Certificate -FilePath $tmp `
        -CertStoreLocation "Cert:\CurrentUser\$store" | Out-Null
}
rdpsign /sha256 $cert.Thumbprint "C:\path\Win11en.rdp"
```

**Leaving RDP without locking the console** (otherwise UI automation in the
session goes blind). Run in cmd on the VM:

```
for /f "skip=1 tokens=3" %s in ('query user %USERNAME%') do ^
tscon.exe %s /dest:console
```

---

## 6. Troubleshooting

**`ssh: connect ... Connection timed out`**

- `Get-Service sshd` must be Running.
- `Get-NetFirewallRule -Name *OpenSSH*`: the default rule applies to the
  Private profile only.
- `Get-NetConnectionProfile` must show `Private`; if `Public`:
  `Get-NetConnectionProfile | Set-NetConnectionProfile -NetworkCategory
  Private`.
- From the client: `Test-NetConnection win11en -Port 22`.

**mstsc takes long to connect**

- The VM name may resolve via mDNS (`win11en.local`) to an IPv6 link-local
  address (`fe80::…`); check with `ping win11en`.
- Fix: reserve an IPv4 address in the router's DHCP and add it to `hosts`
  on the client (see section 5).
- If still slow, test with UDP disabled on the client: set
  `fClientDisableUDP=1` (DWORD) under
  `HKLM\SOFTWARE\Policies\Microsoft\Windows NT\Terminal Services\Client`.

**`ping win11en` times out**

- Inbound ICMP is blocked:
  `Enable-NetFirewallRule -DisplayGroup "File and Printer Sharing"`.

**UAC asks for a password instead of Yes/No**

- The session was started before the user joined Administrators: Sign out
  and sign in again.

**SMB access denied**

- Network profile is Public: switch it to Private (see above).
- Log in as `win11en\smbuser`.

**GUI started, but no window is visible**

- The process runs in a non-interactive session (service / SSH): launch it
  via the `AgentRun` task.

**BSOD `INACCESSIBLE_BOOT_DEVICE` after switching the disk to virtio**

- Install virtio drivers first, then switch the disk controller.
