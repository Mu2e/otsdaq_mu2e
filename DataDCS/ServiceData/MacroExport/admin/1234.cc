//Generated Macro Name:	1234
//Macro Notes: 
//Generated Time: 		Jun-18-2019 01:35:09
//Paste this whole file into an interface to transfer Macro functionality.
{
	char *address 	= new char[universalAddressSize_]{0};	//create address buffer of interface size and init to all 0
	char *data 		= new char[universalDataSize_]{0};		//create data buffer of interface size and init to all 0
	uint64_t macroAddress;		//create macro address buffer (size 8 bytes)
	uint64_t macroData;			//create macro address buffer (size 8 bytes)
	std::map<std::string /*arg name*/,uint64_t /*arg val*/> macroArgs; //create map from arg name to 64-bit number

	// command-#0: Write(0x123 /*address*/,0x123 /*data*/);
	macroAddress = 0x123; memcpy(address,&macroAddress,8);	//copy macro address to buffer
	macroData = 0x123; memcpy(data,&macroData,8);	//copy macro data to buffer
	universalWrite(address,data);

	// command-#1: Read(0x123 /*address*/,data);
	macroAddress = 0x123; memcpy(address,&macroAddress,8);	//copy macro address to buffer
	universalRead(address,data);	memcpy(&macroArgs["outArg1"],data,8); //copy buffer to argument map

	delete[] address; //free the memory
	delete[] data; //free the memory
}