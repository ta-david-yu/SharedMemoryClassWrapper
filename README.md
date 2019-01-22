# SharedMemoryClassWrapper
A class wrapper for one of the POSIX inter-process communication mechanism, shared memory.

# Example
```
struct Data
{
    int value;
    bool user[MAX_PROCESS];
    char msg[2048];
}

int main()
{
    int childCount = 0;
    signal(SIGCHLD, reaper);
    printf("MAX CHILD ID IS %ld\n",sysconf(_SC_CHILD_MAX));
        
    try
    {
        SharedMemory<Data> sharedData("shm_tst");
        Data localData;
        
        
        sharedData.Create();
        sharedData.Attach();
        
        Data& data = *(sharedData.GetData());
        
        fprintf(stdout, "Before VALUE: %d\n", data.value);
        fprintf(stdout, "Before LOCAL VALUE: %d\n", localData.value);
        
        fprintf(stdout, "Before Str: %s\n", data.GetMsg().c_str());
        fprintf(stdout, "Before Str LOCAL: %s\n", localData.GetMsg().c_str());
            
            
        for (size_t i = 0; i < MAX_PROCESS; i++)
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                sharedData.Lock();
                {
                    // shared data
                    data.AddUser(i);
                    
                    // msg data
                    data.SetMsg(std::to_string(data.value));
                
                    // child local data
                    localData.AddUser(i);
                }
                sharedData.Unlock();
                
                //sharedData.Clear();
                
                return;
            }
            else if (pid < 0)
            {
                perror("fork: ");
            }
            else
            {
                // parent local data
                childCount++;
                localData.AddUser(i);
                localData.SetMsg(std::to_string(localData.value));
            }
        }
        
        int status;
        while (wait(&status) > 0);
        
        sharedData.Lock();
        {
            fprintf(stdout, "After VALUE: %d\n", data.value);
            fprintf(stdout, "After LOCAL VALUE: %d\n", localData.value);
            
            fprintf(stdout, "After Str: %s\n", data.GetMsg().c_str());
            fprintf(stdout, "After Str LOCAL: %s\n", localData.GetMsg().c_str());
            
            for (size_t i = 0; i < MAX_PROCESS; i++)
            {
                if (data.user[i] != localData.user[i])
                {
                    fprintf(stderr, "user %d not sync.\n", i);
                }
                assert(data.user[i] == localData.user[i]);
            }
        }
        sharedData.Unlock();
        
        sharedData.Clear();
        
        fprintf(stderr, "Child Count %d !\n", childCount);
        
    }
    catch (std::exception& ex)
    {
        std::cout << "Exception: " << ex.what();
    }
    return;
}
```
