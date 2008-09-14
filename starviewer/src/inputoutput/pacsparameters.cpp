
#include "pacsparameters.h"

namespace udg{

PacsParameters::PacsParameters(QString address, QString port, QString aet, QString aec)
{
    m_address = address;
    m_Port = port;
    m_aeTitle = aet;
    m_aeCalled = aec;
    m_TimeOut = 15000; //establim que per defecte el timeout és de 15000 ms
    m_PacsID = -1;
}

PacsParameters::PacsParameters()
{
    m_PacsID = -1;
}

void PacsParameters::setPacsAddress(QString address)
{
    m_address = address;
}

void PacsParameters::setPacsPort( QString port )
{
     m_Port = port;
}

void PacsParameters::setAELocal( QString aet )
{
     m_aeTitle = aet;
}

void PacsParameters::setAEPacs( QString aec )
{
     m_aeCalled = aec;
}

void PacsParameters::setLocalPort( QString port )
{
    m_LocalPort = port;
}

void PacsParameters::setInstitution( QString inst )
{
    m_Inst = inst;
}

void PacsParameters::setLocation( QString local )
{
    m_Location = local;
}

void PacsParameters::setDescription( QString desc )
{
    m_Desc = desc;
}

void PacsParameters::setDefault( QString def )
{
    m_Default = def;
}

void PacsParameters::setTimeOut( int time )
{
    m_TimeOut = time * 1000; //convertim a ms
}

void PacsParameters::setPacsID( int id )
{
    m_PacsID = id;
}

/*****************************************************************************************/

QString PacsParameters::getPacsAddress() const
{
    return m_address;
}

QString PacsParameters::getPacsPort() const
{
    return m_Port;
}

QString PacsParameters::getAELocal() const
{
    return m_aeTitle;
}

QString PacsParameters::getAEPacs() const
{
    return m_aeCalled;
}

QString PacsParameters::getLocalPort() const
{
    return m_LocalPort;
}

QString PacsParameters::getInstitution() const
{
    return m_Inst;
}

QString PacsParameters::getLocation() const
{
    return m_Location;
}

QString PacsParameters::getDescription() const
{
    return m_Desc;
}

QString PacsParameters::getDefault() const
{
    return m_Default;
}

int PacsParameters::getTimeOut() const
{
    return m_TimeOut;
}

int PacsParameters::getPacsID() const
{
    return m_PacsID;
}

bool PacsParameters::operator ==(const PacsParameters &parameters)
{
    if(    m_aeCalled == parameters.m_aeCalled
        && m_aeTitle == parameters.m_aeTitle
        && m_Port == parameters.m_Port
        && m_address == parameters.m_address
        && m_LocalPort == parameters.m_LocalPort
        && m_Desc == parameters.m_Desc
        && m_Inst == parameters.m_Inst
        && m_Location == parameters.m_Location
        && m_Default == parameters.m_Default
        && m_Location == parameters.m_Location
        && m_PacsID == parameters.m_PacsID
        && m_TimeOut == parameters.m_TimeOut
    )
        return true;
    else
        return false;
}

}

