
#include "scr_file.h"
#include "HardwareSerial.h"

bool SPIFFS_BEGIN() {
	return SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED);
}

File FS_OPEN_FILE(fs::FS &fs, const char *path) {
	File file = fs.open(path);
	if (!file || file.isDirectory()) {
        Serial.println("- failed to open file for reading");
    }
	return file;
}

void SPIFFS_CLOSE_FILE(File file) {
	
}

void SPIFFS_READ_FILE(fs::FS &fs, const char *path)	{
	File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }
    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
}

void SPIFFS_WRITE_FILE(fs::FS &fs, const char * path, const char * message){
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- frite failed");
    }
}