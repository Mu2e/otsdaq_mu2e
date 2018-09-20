#ifndef _ots_TemplateSupervisor_h
#define _ots_TemplateSupervisor_h

#include "otsdaq-core/SOAPUtilities/SOAPMessenger.h"

#include <xdaq/Application.h>
#include <xgi/Method.h>

#include <xoap/MessageReference.h>
#include <xoap/MessageFactory.h>
#include <xoap/SOAPEnvelope.h>
#include <xoap/SOAPBody.h>
#include <xoap/domutils.h>
#include <xoap/Method.h>


#include <cgicc/HTMLClasses.h>
#include <cgicc/HTTPCookie.h>
#include <cgicc/HTMLDoctype.h>
#include <cgicc/HTTPHeader.h>

#include <string>
#include <map>

namespace ots
{


class TemplateSupervisor: public xdaq::Application, public SOAPMessenger
{

public:

    XDAQ_INSTANTIATOR();

    TemplateSupervisor            (xdaq::ApplicationStub * s) ;
    virtual ~TemplateSupervisor   (void);
    void init                  (void);
    void destroy               (void);
    void Default               (xgi::Input* in, xgi::Output* out) ;


private:

};

}

#endif
