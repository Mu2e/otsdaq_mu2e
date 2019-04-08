// Generated Macro Name:	DTC_Reset
// Macro Notes: bring DTC out of broken state into working state
// Generated Time: 		Jan-15-2019 03:35:45
// Paste this whole file into an interface to transfer Macro functionality.
{
	char* address = new char[universalAddressSize_]{
	    0};  //create address buffer of interface size and init to all 0
	char* data = new char[universalDataSize_]{
	    0};                 //create data buffer of interface size and init to all 0
	uint64_t macroAddress;  // create macro address buffer (size 8 bytes)
	uint64_t macroData;     // create macro address buffer (size 8 bytes)
	std::map<std::string /*arg name*/, uint64_t /*arg val*/>
	    macroArgs;  // create map from arg name to 64-bit number

	// command-#0: Write(0x9100 /*address*/,0xa0000000 /*data*/);
	macroAddress = 0x9100;
	memcpy(address, &macroAddress, 8);  // copy macro address to buffer
	macroData = 0xa0000000;
	memcpy(data, &macroData, 8);  // copy macro data to buffer
	universalWrite(address, data);

	// command-#1: Write(0x9118 /*address*/,0x0000003f /*data*/);
	macroAddress = 0x9118;
	memcpy(address, &macroAddress, 8);  // copy macro address to buffer
	macroData = 0x0000003f;
	memcpy(data, &macroData, 8);  // copy macro data to buffer
	universalWrite(address, data);

	// command-#2: Write(0x9100 /*address*/,0x00000000 /*data*/);
	macroAddress = 0x9100;
	memcpy(address, &macroAddress, 8);  // copy macro address to buffer
	macroData = 0x00000000;
	memcpy(data, &macroData, 8);  // copy macro data to buffer
	universalWrite(address, data);

	// command-#3: Write(0x9100 /*address*/,0x10000000 /*data*/);
	macroAddress = 0x9100;
	memcpy(address, &macroAddress, 8);  // copy macro address to buffer
	macroData = 0x10000000;
	memcpy(data, &macroData, 8);  // copy macro data to buffer
	universalWrite(address, data);

	// command-#4: Write(0x9100 /*address*/,0x30000000 /*data*/);
	macroAddress = 0x9100;
	memcpy(address, &macroAddress, 8);  // copy macro address to buffer
	macroData = 0x30000000;
	memcpy(data, &macroData, 8);  // copy macro data to buffer
	universalWrite(address, data);

	// command-#5: Write(0x9100 /*address*/,0x10000000 /*data*/);
	macroAddress = 0x9100;
	memcpy(address, &macroAddress, 8);  // copy macro address to buffer
	macroData = 0x10000000;
	memcpy(data, &macroData, 8);  // copy macro data to buffer
	universalWrite(address, data);

	// command-#6: Write(0x9118 /*address*/,0x00000000 /*data*/);
	macroAddress = 0x9118;
	memcpy(address, &macroAddress, 8);  // copy macro address to buffer
	macroData = 0x00000000;
	memcpy(data, &macroData, 8);  // copy macro data to buffer
	universalWrite(address, data);

	delete[] address;  // free the memory
	delete[] data;     // free the memory
}