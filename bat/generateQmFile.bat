:: 正在生成中文翻译文件
echo.
echo [1/10] generating zh_CN.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_zh_CN.ts -qm ./zh_CN/VmsTranslator_zh_CN.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_zh_CN.qm 
:: -------------------------------------------------------------------
:: Finish
:: -------------------------------------------------------------------

:: -------------------------------------------------------------------
:: 2. 等待1秒
:: -------------------------------------------------------------------
echo.
echo [Waiting] Pause 1s...
timeout /t 1 /nobreak >nul

:: 正在生成西班牙翻译文件
echo.
echo [2/10] generating esp.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_ESP.ts -qm ./es/VmsTranslator_ESP.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_ESP.qm 

:: -------------------------------------------------------------------
:: 2. 等待1秒
:: -------------------------------------------------------------------
echo.
echo [Waiting] Pause 1s...
timeout /t 1 /nobreak >nul

:: 正在生成法语翻译文件
echo.
echo [3/10] generating fr.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_FR.ts -qm ./fr/VmsTranslator_FR.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_FR.qm 

:: -------------------------------------------------------------------
:: 2. 等待1秒
:: -------------------------------------------------------------------
echo.
echo [Waiting] Pause 1s...
timeout /t 1 /nobreak >nul

:: 正在生成泰语翻译文件
echo.
echo [4/10] generating th.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_TH.ts -qm ./th/VmsTranslator_TH.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_TH.qm 

:: -------------------------------------------------------------------
:: 2. 等待1秒
:: -------------------------------------------------------------------
echo.
echo [Waiting] Pause 1s...
timeout /t 1 /nobreak >nul

:: 正在生成德语翻译文件
echo.
echo [5/10] generating de.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_DE.ts -qm ./de/VmsTranslator_DE.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_DE.qm 

:: -------------------------------------------------------------------
:: 2. 等待1秒
:: -------------------------------------------------------------------
echo.
echo [Waiting] Pause 1s...
timeout /t 1 /nobreak >nul

:: 正在生成阿拉伯翻译文件
echo.
echo [6/10] generating ar.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_AR.ts -qm ./ar/VmsTranslator_AR.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_AR.qm 

:: -------------------------------------------------------------------
:: 2. 等待1秒
:: -------------------------------------------------------------------
echo.
echo [Waiting] Pause 1s...
timeout /t 1 /nobreak >nul

:: 正在生成意大利翻译文件
echo.
echo [7/10] generating it.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_IT.ts -qm ./it/VmsTranslator_IT.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_IT.qm 

:: -------------------------------------------------------------------
:: 2. 等待1秒
:: -------------------------------------------------------------------
echo.
echo [Waiting] Pause 1s...
timeout /t 1 /nobreak >nul

:: 正在生成日语翻译文件
echo.
echo [8/10] generating ja.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_JA.ts -qm ./ja/VmsTranslator_JA.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_JA.qm 

:: -------------------------------------------------------------------
:: 2. 等待1秒
:: -------------------------------------------------------------------
echo.
echo [Waiting] Pause 1s...
timeout /t 1 /nobreak >nul

:: 正在生成俄语翻译文件
echo.
echo [9/10] generating ru.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_RU.ts -qm ./ru/VmsTranslator_RU.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_RU.qm 

:: -------------------------------------------------------------------
:: 2. 等待1秒
:: -------------------------------------------------------------------
echo.
echo [Waiting] Pause 1s...
timeout /t 1 /nobreak >nul

:: 正在生成葡萄牙语翻译文件
echo.
echo [10/10] generating pt.qm lrelease...
lrelease ./VmsTsFile/VmsTranslator_PT.ts -qm ./pt/VmsTranslator_PT.qm
if %ERRORLEVEL% neq 0 (
    echo.
    echo [Error] lrelease generate failed  Error code: %ERRORLEVEL%
    goto :error
)
echo [Success] lrelease finish
echo [Success] Generate Success VmsTranslator_PT.qm 

echo.
echo ===== Bat Finish =====
pause
exit /b 0

:error
echo.
echo =====  Error occurs =====
echo Error occurs at: %time%
pause
exit /b 1