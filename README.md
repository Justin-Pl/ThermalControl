# ThermalControl

**[English](#english)** | **[Deutsch](#deutsch)**

---

## English

### About

ThermalControl is a temperature monitoring and control application for a custom Arduino-based interface (ATmega2560, DHT22 sensor, PWM-controlled MOSFET load). It provides:

- Live temperature/PWM graphing
- Three control modes: Manual, Two-Point, and PID
- Firmware flashing directly from the application (STK500v2 protocol)
- Data export/import (measurement data as CSV, console log as TXT, controller configuration as CFG, graph as PNG)

### Download

Pre-built executables are available under **[Releases](../../releases)** – no compiler or toolchain required. Simply download the latest release, extract it, and run `ThermalControl.exe`.

### Building from Source

If you want to build the project yourself instead of using the pre-built release, follow the steps below.

#### Prerequisites

**1. Git**

```powershell
winget install --id Git.Git --source winget
```

**2. MSYS2**

```powershell
winget install --id MSYS2.MSYS2 --source winget
```

**3. Compiler toolchain via MSYS2**

Open **MSYS2 MSYS** from the Start menu (not PowerShell) and run:

```bash
pacman -Syu
```

> Note: On the very first run, this command often terminates the window on its own partway through. This is expected — simply reopen MSYS2 MSYS and run the same command again until it completes without interruption.

Then install the actual build tools:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```

**4. Add MSYS2 to the system PATH**

This step is required — GCC internally invokes helper programs (e.g. `cc1.exe`) that can only locate their own runtime DLLs via the PATH variable.

Open PowerShell **as Administrator** and run:

```powershell
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\msys64\mingw64\bin", "Machine")
```

**Close all open PowerShell/terminal windows and open a new one** for the change to take effect.

**5. Verify the installation**

In a new PowerShell window:

```powershell
gcc --version
cmake --version
ninja --version
git --version
```

Each command should print a version number.

#### Clone and Build

Choose a suitable folder for the project first, e.g. your Documents folder (otherwise you'll end up directly in your user root):

```powershell
cd $HOME\Documents
```

Then clone the repository:

```powershell
git clone https://github.com/Justin-Pl/ThermalControl.git
cd ThermalControl
cmake --preset x64-debug
cmake --build out/build/x64-debug
```

> Note: The first build automatically downloads and compiles raylib as a dependency, which can take several minutes. Subsequent builds are significantly faster.

#### Run

```powershell
cd out\build\x64-debug\ThermalControl
.\ThermalControl.exe
```

---

## Deutsch

### Über das Projekt

ThermalControl ist eine Software zur Temperaturüberwachung und -regelung für ein selbstgebautes Arduino-basiertes Interface (ATmega2560, DHT22-Sensor, PWM-gesteuerte MOSFET-Last). Die Anwendung bietet:

- Live-Anzeige von Temperatur/PWM als Graph
- Drei Regelungsarten: Manuell, Zweipunkt und PID
- Firmware-Flashen direkt aus der Anwendung heraus (STK500v2-Protokoll)
- Datenexport/-import (Messdaten als CSV, Konsolen-Protokoll als TXT, Reglerkonfiguration als CFG, Graph als PNG)

### Download

Fertige, kompilierte Programme findest du unter **[Releases](../../releases)** – kein Compiler oder Build-Werkzeug nötig. Einfach die neueste Version herunterladen, entpacken und `ThermalControl.exe` starten.

### Selbst kompilieren

Falls du das Projekt lieber selbst bauen statt die fertige Release-Version zu nutzen, folge den Schritten unten.

#### Voraussetzungen

**1. Git**

```powershell
winget install --id Git.Git --source winget
```

**2. MSYS2**

```powershell
winget install --id MSYS2.MSYS2 --source winget
```

**3. Compiler-Toolchain über MSYS2 installieren**

Öffne **MSYS2 MSYS** über das Startmenü (nicht PowerShell) und führe aus:

```bash
pacman -Syu
```

> Hinweis: Beim allerersten Ausführen bricht dieser Befehl oft mittendrin ab und schließt das Fenster selbstständig. Das ist normal – öffne MSYS2 MSYS einfach erneut und führe denselben Befehl nochmal aus, bis er ohne Abbruch durchläuft.

Danach die eigentlichen Build-Werkzeuge installieren:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja
```

**4. MSYS2 zur System-PATH hinzufügen**

Dieser Schritt ist zwingend notwendig – GCC ruft intern weitere Hilfsprogramme (z. B. `cc1.exe`) auf, die ihre eigenen Laufzeit-DLLs nur über die PATH-Variable finden.

Öffne PowerShell **als Administrator** und führe aus:

```powershell
[Environment]::SetEnvironmentVariable("Path", $env:Path + ";C:\msys64\mingw64\bin", "Machine")
```

**Schließe danach alle offenen PowerShell-/Terminal-Fenster und öffne ein neues**, damit die Änderung wirksam wird.

**5. Installation prüfen**

In einem neuen PowerShell-Fenster:

```powershell
gcc --version
cmake --version
ninja --version
git --version
```

Alle vier sollten jeweils eine Versionsnummer ausgeben.

#### Klonen und kompilieren

Wähle zunächst einen geeigneten Ordner für das Projekt aus, z. B. deine Dokumente (standardmäßig landest du sonst direkt im Root deines Benutzerordners):

```powershell
cd $HOME\Documents
```

Dann das Repository klonen:

```powershell
git clone https://github.com/Justin-Pl/ThermalControl.git
cd ThermalControl
cmake --preset x64-debug
cmake --build out/build/x64-debug
```

> Hinweis: Der erste Build lädt automatisch raylib als Abhängigkeit herunter und kompiliert es mit – das kann beim allerersten Mal mehrere Minuten dauern. Nachfolgende Builds sind deutlich schneller.

#### Starten

```powershell
cd out\build\x64-debug\ThermalControl
.\ThermalControl.exe
```
