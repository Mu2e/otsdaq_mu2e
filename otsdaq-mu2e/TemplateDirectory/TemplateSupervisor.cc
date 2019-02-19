#include "otsdaq-demo/TemplateDirectory/TemplateSupervisor.h"
#include "otsdaq-core/Macros/CoutMacros.h"
#include "otsdaq-core/MessageFacility/MessageFacility.h"

#include <xdaq/NamespaceURI.h>

#include <iostream>

using namespace ots;

XDAQ_INSTANTIATOR_IMPL(TemplateSupervisor)

//========================================================================================================================
TemplateSupervisor::TemplateSupervisor(xdaq::ApplicationStub* s)
    : xdaq::Application(s), SOAPMessenger(this)
{
	INIT_MF("TemplateSupervisor");
	xgi::bind(this, &TemplateSupervisor::Default, "Default");
	init();
}

//========================================================================================================================
TemplateSupervisor::~TemplateSupervisor(void) { destroy(); }
//========================================================================================================================
void TemplateSupervisor::init(ConfigurationManager* configManager)
{
	// called by constructor
}

//========================================================================================================================
void TemplateSupervisor::destroy(void)
{
	// called by destructor
}

//========================================================================================================================
void TemplateSupervisor::Default(xgi::Input* in, xgi::Output* out)
{
	*out << "<!DOCTYPE HTML><html lang='en'><frameset col='100%' "
	        "row='100%'><frame src='/WebPath/html/Chat.html?urn="
	     << this->getApplicationDescriptor()->getLocalId() << "'></frameset></html>";
}
