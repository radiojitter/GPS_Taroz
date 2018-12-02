# GPS_Taroz  
Using RTLSDR to receive GPS signal in realtime on Microsoft Windows  

# Folders  
*  ./GPS_Taroz/GNSS_Viewer        Contains the software to verify the position reported by the RTLSDR GNSS Software by Taroz. The software named "GNSS_Viewer-CustomerRelease-2.0.318.exe" can connect to GNSS hardware (from Skytrac, etc) using UART/COM port and parse NMEA formatted data to report the User Location, Satellite informations, etc.  
*  ./GPS_Taroz/bin                This folder contains a pre-compiled version of the Taroz's GNSS software. This software can turn the RTLSDR Bias-Tee ON when the Receiver is switched ON and later turn it OFF when not required. Run the software named "gnss-sdrgui.exe" with RTLSDR Connected and GPS Active Antenna connected to the RTLSDR. Once the GNSS is locked the position will be reported using RTKNAVI. The RTKNavi will automatically be started when the main executable named "gnss-sdrgui.exe" is started.  
*  ./GPS_Taroz/Source             This folder contains the modified source code corresponding to the "gnss-sdrgui.exe" executable.  

# Modifications  
* Added support to change RTLSDR gain manually.  
* RTLSDR data receiver thread is made High priority Thread to reduce data loss.  
* Code included to run RTKNAVI on start of the main executable.  
* Code included to switch on RTL Bias-Tee ON at start and switch it OFF when the executable is exiting.  
