// Generated Macro Name:	CAPTAN_OnePulse
// Macro Notes:
// Generated Time: 		Dec-10-2018 03:44:58
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

	// command-#0: Write(0x1001 /*address*/,0x1 /*data*/);
	macroAddress = 0x1001;
	memcpy(address, &macroAddress, 8);  // copy macro address to buffer
	macroData = 0x1;
	memcpy(data, &macroData, 8);  // copy macro data to buffer
	universalWrite(address, data);

	delete[] address;  // free the memory
	delete[] data;     // free the memory
}