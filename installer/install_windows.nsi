; NSIS 一键安装脚本 - 银行柜面双录控件 (Windows 32位)
; 使用 NSIS (Nullsoft Scriptable Install System) - 开源免版税

!define APP_NAME    "银行双录控件"
!define APP_VERSION "1.0.0"
!define EXE_NAME    "DualRecordService.exe"
!define INSTALL_DIR "$PROGRAMFILES32\BankDualRecord"
!define REG_KEY     "SOFTWARE\BankSystem\DualRecord"
!define UNINSTALL_KEY "SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\BankDualRecord"

Name "${APP_NAME} v${APP_VERSION}"
OutFile "BankDualRecord_Setup_v${APP_VERSION}_x86.exe"
InstallDir "${INSTALL_DIR}"
RequestExecutionLevel admin
SetCompressor /SOLID lzma

;─────────────────────────────────────────
; 页面
;─────────────────────────────────────────
!include "MUI2.nsh"
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "docs\LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "SimpChinese"

;─────────────────────────────────────────
; 安装段
;─────────────────────────────────────────
Section "主程序" SecMain
    ; 若旧版本正在运行，先强制结束进程（重新安装场景）
    ExecWait 'taskkill /F /IM "${EXE_NAME}"'
    Sleep 800   ; 等待进程完全退出，避免文件被占用

    SetOutPath "${INSTALL_DIR}"

    ; 程序文件
    File "bin\DualRecordService.exe"
    File "bin\Qt5Core.dll"
    File "bin\Qt5Gui.dll"
    File "bin\Qt5Widgets.dll"
    File "bin\Qt5Network.dll"
    File "bin\Qt5Multimedia.dll"
    File "bin\Qt5MultimediaWidgets.dll"
    File "bin\libssl-1_1.dll"
    File "bin\libcrypto-1_1.dll"
    File /r "bin\platforms\"
    File /r "bin\imageformats\"
    File /r "bin\audio\"

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
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${UNINSTALL_KEY}" "NoRepair"             1

    ; 开机自启 (当前用户)
    WriteRegStr HKCU "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" \
        "${APP_NAME}" '"${INSTALL_DIR}\${EXE_NAME}"'

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
    ; 用 Exec（异步）而非 ExecWait，避免安装程序等待业务进程退出
    Exec '"${INSTALL_DIR}\${EXE_NAME}"'
SectionEnd

;─────────────────────────────────────────
; 卸载段
;─────────────────────────────────────────
Section "Uninstall"
    ; 停止进程
    ExecWait 'taskkill /F /IM "${EXE_NAME}"'

    ; 删除文件
    RMDir /r "${INSTALL_DIR}"

    ; 删除注册表
    DeleteRegKey HKLM "${REG_KEY}"
    DeleteRegKey HKLM "${UNINSTALL_KEY}"
    DeleteRegValue HKCU \
        "SOFTWARE\Microsoft\Windows\CurrentVersion\Run" "${APP_NAME}"

    ; 删除快捷方式
    Delete "$DESKTOP\${APP_NAME}.lnk"
    RMDir /r "$SMPROGRAMS\${APP_NAME}"
SectionEnd
