# BeagleboneBlack-QNX-QT

 QT 4.7.1 QNX binaries uploaded on my google drive.
 
      https://drive.google.com/open?id=0B02sL8YIqsjCNS1ZWFNkQzRRY0E
      ( this was there in the QNX site.. but now the link is removed  (qt_qnx_2011-02-24b.zip))
 
copy the files to the respective QNX installation folder 
       qt_qnx_host_win32_x86 goes to  C:\QNX650\host (mine installed here) 

       qt_qnx_targets\target\qnx6\armle-v7  goes to  C:\QNX650\target\qnx6\armle-v7

        qt_qnx_targets\target\qnx6\usr goes to C:\QNX650\target\qnx6\usr


Building BSP
    a.create an empty project 
    b.File -> import ->General ->Existing Projects in to Workspace -> File system (select the unzipped folder ) 
    c.build 

QT Application building (on Windows)
   
   a. cmd <-   go to the project folder
   b. qmake -spec unsupported/qws/qnx-armv7-g++
   c. make 
   For debug option 
   a.qmake -spec unsupported/qws/qnx-armv7-g++ CONFIG+=debug
   b. make or make CONFIG+=debug

Running QML Application 
    
    a.use qmlviewer Demo.qml
          or 
     b. cEasy easyQML.qml

QT Debugging from IDE 
 
     a. QNX IDE -> File ->New -> QNX C++ Project -> build variants -> armV7 
     b. Right click on the project -> properties
        Select Compiler Tab ->  Extra Include path 

     c.Add from -> "Disk"  -  then "apply "

        [QNX Directory ] /target/qnx6/armle-v7/usr/lib/qt4/imports
        [QNX Directory ]/target/qnx6/armle-v7/usr/lib/qt4/plugins
        [QNX Directory ]/target/qnx6/usr/include/qt4


     e.Linker Tab 
         category  "Extra library  "  click "Add"     and type
         QtGui
         QtCore
         
     f.  debug configuration-> Environment variable
          TMPDIR=/ramdisk
          XDG_CONFIG_HOME=/ramdisk

     f.click "apply "  , click "ok"   , Now "build project" can compile the QT source code.



Note :first QT application should be accompanied with '-qws'
 qtdemo -qws &
 
Uboot /Flashing:

You can download the files from QNX site / uploaded here for convenience 
           
          https://drive.google.com/open?id=0B02sL8YIqsjCWlpRWVVTVWpkZ00
          
          use TI_SDCard_boot_utility_v1_0.exe to copy the MLO file , on windows Run as administrator 
 
 Screen Resolution :
 
          Screen resolution set to 800x480 , To edit this go to bsp-ti-beaglebone-src\src\hardware\devg\omapl1xx\omapl1xx.h
          
 


 

