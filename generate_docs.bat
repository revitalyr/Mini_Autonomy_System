@echo off
echo Generating Doxygen documentation...

REM Create docs directory if it doesn't exist
if not exist docs mkdir docs

REM Run doxygen
doxygen Doxyfile

REM Check if documentation was generated successfully
if exist docs\html\index.html (
    echo Documentation generated successfully!
    echo Open docs\html\index.html to view the documentation
    start docs\html\index.html
) else (
    echo Failed to generate documentation
    pause
)
