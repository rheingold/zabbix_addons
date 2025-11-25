@echo off
setlocal enabledelayedexpansion

:: ======================================================
:: Usage:
::   setini.bat "file.ini" "KeyName" "NewValue" ["SectionName"]
:: ======================================================
::echo ##::%TODAY% %NOW% Previous line updated by install script
for /f %%a in ('wmic os get LocalDateTime ^| find "."') do set dt=%%a
set "TODAY=%dt:~0,4%-%dt:~4,2%-%dt:~6,2%"
set "NOW=%dt:~8,2%:%dt:~10,2%:%dt:~12,2%"



if "%~3"=="" (
    echo Usage: %~nx0 "file.ini" "Key" "Value" ["SectionName"]
    exit /b 1
)

set "myname=%~f0"
set "op=%~1"
set "inifile=%~2"
set "key=%~3"
set "defaultconfpath=%~3"
set "newval=%~4"
set "instnote=%~5"
set "section=%~6"
set "macroparam=%~7"
set "tempfile=%inifile%.tmp"
::set "UPDATEMSG="
set "UPDATEMSG=%INSTNOTE% %TODAY% %NOW% Previous line updated by install script"
:: Define a newline variable
(set NL=^

)




:MAIN
if not exist "%inifile%" (
	echo %iniFile% does not exist
	exit /b
)
set "arg="


if /I "%op%"=="set" (
	call :op_set
) else if /I "%op%"=="get" (
	call :op_get
) else if /I "%op%"=="input" (
	call :op_input
) else if /I "%op%"=="proc" (
	call :op_process
)
exit /b

:op_input
:: Interactive input mode - reads comment metadata and prompts user
:: Usage: inied.bat input "file.ini" "KeyName" "SectionName"
:: Returns the user input value based on ;;INST|mode|prompt comment
set "section=%newval%"
set "prompttext="
set "inputmode="
set "currentvalue="
set "insection=0"

:: Read the file to find the key and its comment
for /f "usebackq tokens=*" %%A in ("%inifile%") do (
    set "line=%%A"
    
    :: Check if we're entering the target section
    if "!line:~0,1!"=="[" (
        set "insection=0"
        if /I "!line!"=="[%section%]" set "insection=1"
    )
    
    :: If in target section, look for ;;INST comment
    if "!insection!"=="1" (
        if "!line:~0,6!"==";;INST" (
            :: Parse comment: ;;INST|mode|prompttext
            for /f "tokens=2,3 delims=|" %%B in ("!line!") do (
                set "inputmode=%%B"
                set "prompttext=%%C"
            )
        )
        
        :: Check if this is our key line
        for /f "tokens=1* delims==" %%B in ("!line!") do (
            set "linekey=%%B"
            set "linekey=!linekey: =!"
            if /I "!linekey!"=="%key%" (
                set "currentvalue=%%C"
                set "currentvalue=!currentvalue: =!"
                goto :input_prompt
            )
        )
    )
)

:input_prompt
:: Prompt user based on mode
if "%inputmode%"=="" (
    if defined currentvalue echo !currentvalue!
    exit /b 0
)

if "%inputmode%"=="c" (
    :: Constant - don't prompt, just return current value
    if defined currentvalue echo !currentvalue!
    exit /b 0
)

if "%inputmode%"=="m" (
    :: Mandatory - don't prompt user, just return current value
    if defined currentvalue echo !currentvalue!
    exit /b 0
)

if "%inputmode%"=="d" (
    :: Default - user can press ENTER to accept default
    set "userinput="
    set /p "userinput=%prompttext% (default=%currentvalue%): " 
    if "!userinput!"=="" set "userinput=!currentvalue!"
    if defined userinput echo !userinput!
    exit /b 0
)

if "%inputmode%"=="am" (
    :: Append+Mandatory - append user input to current value
    set "userinput="
    set /p "userinput=%prompttext% %currentvalue%" 
    if "!userinput!"=="" (
        echo ERROR: Input required! >&2
        goto :input_prompt
    )
    set "finalvalue=!currentvalue!!userinput!"
    if defined finalvalue echo !finalvalue!
    exit /b 0
)

:: Default fallback
if defined currentvalue echo !currentvalue!
exit /b 0


:op_get
set "r="
set "section=%newval%"
for /f "usebackq tokens=1* delims=:" %%A in (`findstr /n "^" "%inifile%"`) do (
	set "line=%%B"
	set "trim=!line: =!"
    REM Skip empty lines or comments
    IF NOT "!trim!"=="" IF NOT "!trim:~0,1!"=="#" IF NOT "!trim:~0,1!"==";" (
        REM Check if line is a section
        IF "!trim:~0,1!"=="[" (
			IF "!trim:~-1!"=="]"  (
				SET "currentSection=!trim:~1,-1!"
				IF /I "!currentSection!"=="%section%" (
					SET "inSection=1"
				) ELSE (
					SET "inSection=0"
					if /i "%section%"=="" (
						set "inSection=1"
					)
				)
			)
        ) else (
            REM If in the correct section, look for the key
            IF "!inSection!"=="1" (
                FOR /F "tokens=1* delims==:" %%K IN ("!trim!") DO (
                    SET "currentKey=%%K"
                    SET "currentValue=%%L"
                    IF /I "!currentKey!"=="%key%" (
                        SET "value=!currentValue!"
                    )
                )
            )
        )
    )
)

::ECHO Value of [%section%] %key% = %value%
ECHO(%value%
exit /b

:op_set
set "found_section="
set "updated="

echo FILE: %inifile%
echo  KEY: %key%
echo SECT: %section%

(for /f "usebackq tokens=1* delims=:" %%A in (`findstr /n "^" "%inifile%"`) do (
    set "line=%%B"
	
	set "trim=!line: =!"
	set "output_this_line=1"
	
	REM Process all lines including empty ones to preserve file structure
	REM Empty lines will be output as-is without processing
	
	if "!prevlinereplaced!"=="1" (
		::Check that the read line contains an installer note, which has been put there already in the output by the previous call of CheckAndReplace subroutine
		set "prevlinereplaced=0"
		set "check=!trim:%INSTNOTE%=!"
		REM If this is a comment line from previous update, just output it and skip to next line
		if not "!check!"=="!trim!" (
			set "newline=!line!"
			REM No doubling needed - delayed expansion preserves %%
			echo(!newline!
			set "output_this_line=0"
		) else (
			set "check=!trim!"
		)
	) else (
		set "check=!trim!"
	)

	if "!output_this_line!"=="1" if "!check!"=="!trim!" (
		REM Only process non-empty lines for section detection and key replacement
		if not "!trim!"=="" (
			:: Check for section start [SectionName]
			if defined section (
				if "!trim!"=="[%section%]" (
					set "found_section=1"
				) else (
					if defined found_section (
						if "!trim:~0,1!"=="[" set "found_section="
					)
				)
			)
		
			set "newline=!line!"
			set "line_was_modified=0"
			:: Update only if inside target section (or if no section specified)
			if defined section (
				if defined found_section (
					REM Set global variable for CheckAndReplace to avoid %~1 halving
					set "CHECKLINE=!line!"
					call :CheckAndReplace
					if "!prevlinereplaced!"=="1" set "line_was_modified=1"
				)
			) else (
					REM Set global variable for CheckAndReplace to avoid %~1 halving
					set "CHECKLINE=!line!"
					call :CheckAndReplace
					if "!prevlinereplaced!"=="1" set "line_was_modified=1"
			)
			REM Only double %% for modified lines (delayed expansion halves during echo)
			REM Skip doubling if NO_PERCENT_DOUBLING flag is set (for PROC operation recursive calls)
			if "!line_was_modified!"=="1" if not defined NO_PERCENT_DOUBLING if not "!newline!"=="" set "newline=!newline:%%=%%%%!"
		) else (
			REM Empty line - output as-is without processing
			set "newline=!line!"
		)
		echo(!newline!
	)
	)) > "%tempfile%"
move /y "%tempfile%" "%inifile%" >nul
exit /b

:CheckAndReplace
    REM Don't use setlocal to avoid %% halving when returning values
    set "prevlinereplaced=0"
	REM Use global variable CHECKLINE to preserve %%
	set "line=!CHECKLINE!"
	if "!line!"=="" set "line=!arg!"
	set "prefix="
	rem -- Loop to peel off leading spaces --
    :loopSpaces
    if defined line if "!line:~0,1!"==" " (
        set "prefix=!prefix! "
        set "line=!line:~1!"
        goto loopSpaces
    )
    rem -- Now !line! has no leading spaces. Split at first '=' --

    REM Check if line contains '=' by string substitution test
    set "testline=!line:==!"
    
    if "!testline!"=="!line!" (
        REM No '=' found, just preserve the line as-is
        set "prevlinereplaced=0"
    ) else (
        for /f "tokens=1* delims==" %%A in ("!line!") do (
            set "keyname=%%A"
            set "keyname=!keyname: =!"
            
            if /i "!keyname!"=="%key%" (
                set "line=%key%=%newval%"
                set "prevlinereplaced=1"
            ) else (
                REM Preserve original line format for non-matching keys
                REM Don't reconstruct from %%A/%%B as this creates bare = lines
                set "prevlinereplaced=0"
            )
        )
    )
	if "%UPDATEMSG%"=="" (
		set "prevlinereplaced=0"
	)
	::^^^ if we have not update mesage, then the entire add-and-skip-read-line mechanism is disabled here

	REM Set newline directly without endlocal to preserve %%
	if "!prevlinereplaced!"=="1" (
		set "newline=!prefix!!line!!NL!!prefix!# %UPDATEMSG%"
		set "prevlinereplaced=1"
	) else (
		set "newline=!prefix!!line!"
		set "prevlinereplaced=0"
	)	
exit /b


:op_process
SET "currentFile="

REM Prepare macros string for expansion. macroparam may be:
REM  - empty : no macro expansion beyond defaults
REM  - "auto" : read macros from [inied.bat-macros] section in the same ini file
REM  - explicit string like NAME=Value|OTHER=Value2
set "macros_str="
if defined macroparam (
    if /I "%macroparam%"=="auto" (
        call :CollectMacrosFromIni "%inifile%" macros_str
    ) else (
        set "macros_str=%macroparam%"
    )
)

FOR /f "usebackq tokens=*" %%A IN ("%iniFile%") DO (
	SET "line=%%A"
    REM Trim spaces - only trailing ending, leading spaces is done by tokens=*
	call :TrimTrailing line
    REM Skip empty lines and comments
    IF NOT "!line!"=="" IF NOT "!line!"==" " IF NOT "!line:~0,1!"=="#" IF NOT "!line:~0,1!"==";" (
        REM Check for section (directory path)
        IF "!line:~0,1!"=="[" ( 
			IF "!line:~-1!"=="]" (
				SET "currentFile=!line:~1,-1!"
				REM Skip bootstrap-silent-install-only section - it's for install.bat only, not for config files
				IF /I "!currentFile!"=="bootstrap-silent-install-only" (
					SET "currentFile="
                ) ELSE (
                        if "!currentFile!"=="default" set "currentfile=%key%"
                        rem Expand macros in target file path (uses macros_str prepared below)
                        call :ExpandMacros "!currentFile!" currentFile "!macros_str!"
                        ECHO Processing directory: !currentFile!
                    )
			)
        ) ELSE (
            REM Must be key=value line
            FOR /F "tokens=1* delims==: " %%K IN ("!line!") DO (
                SET "k=%%K"
                SET "v=%%L"
                if not "!currentFile!"=="" if not "!k!"=="" if not "!v!"=="" (
                    rem Expand macros in the value using provided macros_str
                    call :ExpandMacros "!v!" expanded_v "!macros_str!"
                    rem Set flag to prevent %% doubling during recursive call
                    set "NO_PERCENT_DOUBLING=1"
                    call "%myname%" set "!currentFile!" "!k!" "!expanded_v!" "!INSTNOTE!"
                    set "NO_PERCENT_DOUBLING="
                )

            )
        )
    )
)
exit /b

:TrimTrailing
setlocal EnableDelayedExpansion
set "var=!%1!"
:Loop
if "!var:~-1!"==" " set "var=!var:~0,-1!" & goto Loop
endlocal & set "%1=%var%"
goto :EOF

:CollectMacrosFromIni
:: Collects key=value pairs from [inied.bat-macros] section and returns a pipe-separated list
:: Usage: call :CollectMacrosFromIni "file.ini" outVar
setlocal EnableDelayedExpansion
set "ini=%~1"
set "out="
set "insection=0"
for /f "usebackq tokens=1* delims=:" %%A in (`findstr /n "^" "%ini%"`) do (
    set "ln=%%B"
    set "trim=!ln!"
    if "!trim:~0,1!"=="[" (
        set "sec=!trim:~1,-1!"
        if /I "!sec!"=="inied.bat-macros" (
            set "insection=1"
        ) else (
            if "!insection!"=="1" set "insection=0"
        )
    ) else (
        if "!insection!"=="1" (
            if not "!trim!"=="" if not "!trim:~0,1!"=="#" if not "!trim:~0,1!"==";" (
                for /f "tokens=1* delims==" %%K in ("!trim!") do (
                    set "k=%%K"
                    set "v=%%L"
                    set "out=!out!!k!=!v!|"
                )
            )
        )
    )
)
endlocal & set "%~2=%out%"
goto :EOF

:ExpandMacros
:: Expands macros in format %%MACRONAME%% using an explicit macro map string
:: Usage: call :ExpandMacros "input_string" result_var "NAME=Val|OTHER=Val2"
setlocal EnableDelayedExpansion
set "input=%~1"
set "macros_in=%~3"
set "output=!input!"
if "!macros_in!"=="" (
    endlocal & set "%~2=%output%"
    goto :EOF
)
:: Iterate through pipe-separated NAME=VALUE pairs
:ExpandLoop
set "pair="
set "macros_rest="
for /f "tokens=1* delims=|" %%A in ("!macros_in!") do (
    set "pair=%%A"
    set "macros_rest=%%B"
)
if not "!pair!"=="" (
    set "mname="
    set "mval="
    for /f "tokens=1* delims==" %%K in ("!pair!") do (
        set "mname=%%K"
        set "mval=%%L"
    )
    set "mname=!mname: =!"
    if not "!mname!"=="" if not "!mval!"=="" (
        REM Use PowerShell for replacement - input has single % when passed to PowerShell
        for /f "delims=" %%R in ('powershell -NoProfile -Command "$s='!output!'; $s -replace '%%!mname!%%','!mval!'"') do set "output=%%R"
    )
)
if defined macros_rest (
    set "macros_in=!macros_rest!"
    goto ExpandLoop
)
endlocal & set "%~2=%output%"
goto :EOF
