#pragma once

#include <vector>
#include <utility>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>

#include <string>

#define SEMA_NAME "/semaKami3_"

#define DEBUG
#define NO_MSG

#ifdef NO_MSG

#define LOCK_SHM(x)                                                                 \
 do                                                                                 \ 
 {                                                                                  \
     x.Lock();                                                                      \
 } while(false)

#define UNLK_SHM(x)                                                                 \
 do                                                                                 \ 
 {                                                                                  \
     x.Unlock();                                                                    \
 } while(false)

#else

#define LOCK_SHM(x)                                                                 \
 do                                                                                 \ 
 {                                                                                  \
     x.Lock();                                                                      \
     fprintf(stderr, "  + LOCK BY: %d At %s (%d) \n", getpid(), __FILE__, __LINE__);\
 } while(false)

#define UNLK_SHM(x)                                                                 \
 do                                                                                 \ 
 {                                                                                  \
     x.Unlock();                                                                    \
     fprintf(stderr, "  - UNLK BY: %d At %s (%d) \n", getpid(), __FILE__, __LINE__);\
 } while(false)

 #endif
 
namespace DYE
{
    namespace Networking 
    {
		//==========================================
		//	SharedMemory exception
        //==========================================
        class SharedMemoryException: public std::exception
        {
        public:
            /**
           *   Construct a SharedMemoryException with a explanatory message.
           *   @param message explanatory message
           *   @param bSysMsg true if system message (from strerror(errno))
           *   should be postfixed to the user provided message
           */
            SharedMemoryException(const std::string &message, bool bSysMsg = false) throw();
         
         
            /** Destructor.
             * Virtual to allow for subclassing.
             */
            virtual ~SharedMemoryException() throw ();
         
            /** Returns a pointer to the (constant) error description.
             *  @return A pointer to a \c const \c char*. The underlying memory
             *          is in posession of the \c Exception object. Callers \a must
             *          not attempt to free the memory.
             */
            virtual const char* what() const throw () {  return m_sMsg.c_str(); }
         
        protected:
            std::string m_sMsg;
        };
        
        
		//==========================================
		//	SharedMemory template
        //==========================================
        template<typename T>
        class SharedMemory
        {
        public:
            enum CreateMode
            {
                CRead = O_RDONLY,
                CWrite = O_RDWR
            };
            
            enum AttachMode
            {
                ARead = PROT_READ,
                AWrite = PROT_WRITE
            };
            
			//==========================================
			//	memeber/variable
            //==========================================
            static const std::string s_cSemaphoreNamePrefix;
            
            T* m_pData = nullptr;
            sem_t* m_SemID = nullptr;
            
            size_t m_Size = 0;
            int m_ShmID = -1;
            std::string m_Name;
            std::string m_SemaphoreName;
            
			//==========================================
			//	method
            //==========================================
        public:
            bool Create(int mode = CreateMode::CWrite);
            bool Attach(int mode = AttachMode::ARead | AttachMode::AWrite);
            bool Detach();
            
            bool Lock();
            bool Unlock();
            
            void Clear();
            
			//==========================================
			//	getter/setter
            //==========================================
        public:
            inline int GetFileDescriptor() const { return m_ShmID; } 
            T* GetData() { return m_pData; }
            const T* GetData() const { return m_pData; }
            
			//==========================================
			//	constructor/destructor
            //==========================================
        public:
            SharedMemory(const std::string& name);
            ~SharedMemory();
        };
        
		//==========================================
		//	SharedMemory implementation
        //==========================================
        template<typename T>
        const std::string SharedMemory<T>::s_cSemaphoreNamePrefix = SEMA_NAME;
        
        template<typename T>
        SharedMemory<T>::SharedMemory(const std::string& name) : m_Name(name)
        {
            // create semaphore
            m_SemaphoreName = s_cSemaphoreNamePrefix + name;
            m_SemID = sem_open(m_SemaphoreName.c_str(), O_CREAT, S_IRUSR | S_IWUSR, 1);
            
            if (SEM_FAILED == m_SemID)
            {
#ifdef DEBUG
                perror("sem_open: ");
#endif
            }
            
            fprintf(stderr, "SemaName: %s\n", m_SemaphoreName.c_str());
            
            m_Size = sizeof(T);
        }
        
        template<typename T>
        SharedMemory<T>::~SharedMemory()
        {
            // u should clean the memory manually
            // Clear();
        }
        
        template<typename T>
        bool SharedMemory<T>::Create(int mode /*= Create::CWrite*/)
        {
            m_ShmID = shm_open(m_Name.c_str(), O_CREAT | mode, S_IRUSR | S_IWUSR);
            if (m_ShmID < 0)
            {
                switch(errno)
                {
                case EACCES:
                   throw SharedMemoryException("Permission Exception ");
                   break;
                case EEXIST:
                   throw SharedMemoryException("Shared memory object specified by name already exists.");
                   break;
                case EINVAL:
                   throw SharedMemoryException("Invalid shared memory name passed.");
                   break;
                case EMFILE:
                   throw SharedMemoryException("The process already has the maximum number of files open.");
                   break;
                case ENAMETOOLONG:
                   throw SharedMemoryException("The length of name exceeds PATH_MAX.");
                   break;
                case ENFILE:
                   throw SharedMemoryException("The limit on the total number of files open on the system has been reached");
                   break;
                default:
                   throw SharedMemoryException("Invalid exception occurred in shared memory creation");
                   break;
                }
                return false;
            }
            
            ftruncate(m_ShmID, m_Size);
            return true;
        }
        
        template<typename T>
        bool SharedMemory<T>::Attach(int mode /*= AttachMode::ARead | AttachMode::AWrite*/)
        {
            // map required memory to file object
            m_pData = (T*)mmap(NULL, m_Size, mode, MAP_SHARED, m_ShmID, 0);
            if (m_pData == nullptr)
            {
                throw SharedMemoryException("Exception in attaching the shared memory region");
                return false;
            }
            // copy data to initialize
            T data;
            memcpy((void*)m_pData, (void*)&data, sizeof(T));
            return true;
        }
        
        template<typename T>
        bool SharedMemory<T>::Detach()
        {
            // release mapped memory
            bool ret = (munmap(m_pData, m_Size) == 0);
            m_pData = nullptr;
            return ret;
        }
            
        template<typename T>
        bool SharedMemory<T>::Lock()
        {
            bool ret = (sem_wait(m_SemID) == 0);
            return ret;
        }
        
        template<typename T>
        bool SharedMemory<T>::Unlock()
        {
            bool ret = (sem_post(m_SemID) == 0);
            return ret;
        }
            
        template<typename T>
        void SharedMemory<T>::Clear()
        {
            std::string s_pid = "[PID:" + std::to_string(getpid()) + "]";
            // unlink shared memory file object if it is valid
            if (m_ShmID != -1)
            {
                if (shm_unlink(m_Name.c_str()) < 0)
                {
#ifdef DEBUG
                    s_pid += " shm_unlink: " + std::string(std::strerror(errno));
                    fprintf(stderr, "%s\n",s_pid.c_str());
#endif
                }
            }
            
            // release semaphore if exists
            if (m_SemID != nullptr)
            {
                // close a named semaphore
                if (sem_close(m_SemID) < 0)
                {
#ifdef DEBUG
                    s_pid += " sem_close: " + std::string(std::strerror(errno));
                    fprintf(stderr, "%s\n",s_pid.c_str());
#endif
                }
                
                // release named semaphore file object
                if (sem_unlink(m_SemaphoreName.c_str()) < 0)
                {
#ifdef DEBUG
                    s_pid += " sem_unlink: " + std::string(std::strerror(errno));
                    fprintf(stderr, "%s\n",s_pid.c_str());
#endif
                }
            }
        }
    }
}


