VISUAL C++ SERVICE TEMPLATE
===========================

A modern and simplified approach to handeling windows service in C++.  
It abstracts away most of the tedious work, all you have to do is subclass WindowsService and define a worker.  

Look at example.cpp for a complete example/template.  
Requires VS2015 (C++ 11) or later. (Or equivalent build tools)  

Compiling from the commandLine
------------------------------

Simply call:  
`cl "example.cpp" "Advapi32.lib" /W4 /EHsc`  

Testing the Service
-------------------

Following steps should be executed in powershell:  
1) Create a new service:  
`New-Service -Name test -BinaryPathName full_path_to_exe`  
NOTE: the full path to the executable is required.  
2) Start the service:  
`Start-Service -Name test`  
3) Stop the service:  
`Stop-Service -Name test`  
4) Delete the service:  
`sc.exe delete test`  
NOTE: Make sure you typed sc.exe and not sc.  
      sc is an allias for Set-Content in powershell, not sc.exe.  
