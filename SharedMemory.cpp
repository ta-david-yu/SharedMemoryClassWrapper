#include "SharedMemory.h"

namespace DYE
{
    namespace Networking 
    {
		//==========================================
		//	SharedMemory exception
        //==========================================
        SharedMemoryException::SharedMemoryException(const std::string &message, bool bSysMsg/* = false*/) throw()
        {
        }
        
        SharedMemoryException::~SharedMemoryException() throw()
        {
        }
    }
}
