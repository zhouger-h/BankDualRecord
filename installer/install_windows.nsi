; NSIS 一键安装脚本 - 银行柜面双录控件 (Windows 32位)
; 使用 NSIS (Nullsoft Scriptable Install System) - 开源免版税

!define APP_NAME    "银行双录控件"
!define APP_VERSION "1.0.0"
!define EXE_NAME    "DualRecordService.exe"
!define INSTALL_DIR "$PROGRAMFILES32\BankDualRecord"
!define REG_KEY     "SOFTWARE\BankSystem\DualRecord"
!define UNINSTALL_KEY "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BankDualRecord"

; Base directory (parent of installer/)

!cd ".."


Name "${APP_NAME} v${APP_VERSION}"
OutFile "BankDualRecord_Setup_v${APP_VERSION}_x86.exe"
InstallDir "${INSTALL_DIR}"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

; 安装程序图标
!define MUI_ICON "installer_icon.ico"
!define MUI_UNICON "installer_icon.ico"

;─────────────────────────────────────────
; 页面
;─────────────────────────────────────────
!include "MUI2.nsh"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "docs\LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"

;─────────────────────────────────────────
; 安装段
;─────────────────────────────────────────
Section "主程序" SecMain
    ; 若旧版本正在运行，先强制结束进程（重新安装场景）
    nsExec::ExecToLog 'taskkill /F /IM "${EXE_NAME}"'
    Sleep 800   ; 等待进程完全退出，避免文件被占用

    SetOutPath "${INSTALL_DIR}"

    ; ── 程序文件（bin\ 目录由 build_windows.bat 通过 windeployqt 自动生成）──
    ; EXE
    File "bin\${EXE_NAME}"
    ; Qt 核心依赖
    File "bin\Qt5Core.dll"
    File "bin\Qt5Gui.dll"
    File "bin\Qt5Widgets.dll"
    File "bin\Qt5Network.dll"
    File "bin\Qt5Multimedia.dll"
    File "bin\Qt5MultimediaWidgets.dll"
    ; OpenSSL
    File "bin\libssl-1_1.dll"
    File "bin\libcrypto-1_1.dll"
    ; libssh2（SFTP 支持）
    File "bin\libssh2.dll"
    ; MinGW 运行时
    File "bin\libgcc_s_dw2-1.dll"
    File "bin\libstdc++-6.dll"
    File "bin\libwinpthread-1.dll"
    ; Qt 插件目录
    File /r "bin\platforms\*.*"
    File /r "bin\imageformats\*.*"
    File /r "bin\audio\*.*"
    File /r "bin\styles\*.*"
    ; Qt 资源
    File /r "bin\translations\*.*"

    ; 创建工作目录
    CreateDirectory "$PROFILE\BankDualRecord\videos"
    CreateDirectory "$PROFILE\BankDualRecord\logs"

    ; 写注册表（安装信息）
    WriteRegStr   HKLM "${REG_KEY}" "InstallDir"  "${INSTALL_DIR}"
    WriteRegStr   HKLM "${REG_KEY}" "Version"     "${APP_VERSION}"
    WriteRegStr   HKLM "${REG_KEY}" "ExePath"     "${INSTALL_DIR}\${EXE_NAME}"

    ; 卸载信息
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayName"          "${APP_NAME}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayVersion"       "${APP_VERSION}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "UninstallString"      "${INSTALL_DIR}\Uninstall.exe"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "Publisher"            "BankSystem"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "InstallLocation"      "${INSTALL_DIR}"
    WriteRegStr   HKLM "${UNINSTALL_KEY}" "DisplayIcon"          "${INSTALL_DIR}\${EXE_NAME}"
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"             1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "EstimatedSize"        50000

    ; 开机自启 (当前用户 Run 键)
    WriteRegStr HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" \
        "BankDualRecord" '"${INSTALL_DIR}\${EXE_NAME}"'

    ; 创建桌面快捷方式
    CreateShortcut "$DESKTOP\${APP_NAME}.lnk" "${INSTALL_DIR}\${EXE_NAME}" \
        "" "${INSTALL_DIR}\${EXE_NAME}" 0

    ; 创建开始菜单
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" \
        "${INSTALL_DIR}\${EXE_NAME}" "" "${INSTALL_DIR}\${EXE_NAME}" 0
    CreateShortcut  "$SMPROGRAMS\${APP_NAME}\卸载.lnk" \
        "${INSTALL_DIR}\Uninstall.exe"

    ; 生成卸载程序
    WriteUninstaller "${INSTALL_DIR}\Uninstall.exe"

    ; 安装完成后立即启动程序（首次安装 / 重新安装 均自动拉起）
    Exec '"${INSTALL_DIR}\${EXE_NAME}"'
SectionEnd

;─────────────────────────────────────────
; 卸载段
;─────────────────────────────────────────
Section "Uninstall"
    ; 停止进程
    nsExec::ExecToLog 'taskkill /F /IM "${EXE_NAME}"'
    Sleep 500

    ; 删除安装目录
    RMDir /r "${INSTALL_DIR}"

    ; 删除用户数据目录（可选，保留配置）
    ; RMDir /r "$PROFILE\BankDualRecord"

    ; 删除注册表
    DeleteRegKey HKLM "${REG_KEY}"
    DeleteRegKey HKLM "${UNINSTALL_KEY}"
    DeleteRegValue HKCU \
        "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "BankDualRecord"

    ; 删除快捷方式
    Delete "$DESKTOP\${APP_NAME}.lnk"
    RMDir /r "$SMPROGRAMS\${APP_NAME}"
SectionEnd
