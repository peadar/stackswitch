#include <exception>
#include <cstring>
#include <string>
#include <cstdlib>
#include <csetjmp>

class Runnable {
public:
    virtual void run() const = 0;
};

class StackSwitch {
    char *buf; // A buffer for the stack
    char *scratch;
    char *tosp, *bosp;
    const Runnable *running;
    std::jmp_buf enter;
    std::jmp_buf leave;
    void copystack(char *);
public:
    void setstack(char *);
    StackSwitch(size_t size);
    ~StackSwitch();
    void invoke(const Runnable &r);
};

// wrapper for throwing after a syscall fails.
class syserror : public std::exception {
    const char *call;
    int err;
    std::string whatText;
public:
    ~syserror() throw() {}
    syserror(const char *call_, int err_) throw()
        : call(call_)
        , err(err_)
        , whatText(std::string(call) + " failed: " + strerror(err))
    {}
    const char *what() const throw() {
        return whatText.c_str();
    }
};

// macro for calling pthread method and throwing an exception if it fails.
#define PTCALL(func, arglist) \
    { int rc = func arglist; \
    if (rc != 0) \
        throw syserror(#func, rc); }
