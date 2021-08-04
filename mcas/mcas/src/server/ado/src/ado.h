#ifndef __ADO_H__
#define __ADO_H__

#include <ado_proxy.h>

class IExecutor {
public:
  IExecutor() {}
  virtual ~IExecutor() {}
  virtual status_t execute() = 0;
};

class Dummy : public IExecutor {
public:
  Dummy();
  virtual ~Dummy();
  virtual status_t execute() override;
};

#endif
