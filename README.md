# Antimalware Core Service

This repository is set up with **GitHub Actions** to automate the build of the `AntimalwareCore.exe`.

## 🚀 How it Works
Whenever you push changes to `ass.cpp`, GitHub will:
1. Spin up a Windows runner.
2. Compile the code using `cl.exe` (MSVC) with static linking (`/MT`).
3. Upload the final `.exe` as a build artifact.

## 📂 Project Structure
- `ass.cpp`: The main C++ source code (Stealth mode enabled).
- `.github/workflows/build.yml`: The automation script for GitHub Actions.

## 🛠️ Usage
1. Push your code to GitHub.
2. Go to the **Actions** tab.
3. Select the latest run.
4. Download the `Antimalware-Core-Service` artifact from the bottom of the page.

## 🔒 Build Features
- **Static Linking**: No DLL requirements.
- **Subsystem Windows**: No console window appears when running.
- **O2 Optimization**: Maximized performance and minimized file size.
