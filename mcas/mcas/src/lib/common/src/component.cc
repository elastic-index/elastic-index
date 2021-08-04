/*
   eXokernel Development Kit (XDK)

   Samsung Research America Copyright (C) 2013

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.

   As a special exception, if you link the code in this file with
   files compiled with a GNU compiler to produce an executable,
   that does not cause the resulting executable to be covered by
   the GNU Lesser General Public License.  This exception does not
   however invalidate any other reasons why the executable file
   might be covered by the GNU Lesser General Public License.
   This exception applies to code released by its copyright holders
   in files containing the exception.
*/

/*
  Authors:
  Copyright (C) 2014, Daniel G. Waddington <daniel.waddington@acm.org>
*/

#include <common/component.h>
#include <assert.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <common/errors.h>
#include <common/logging.h>
#include <common/exceptions.h>
#include <component/base.h>
#include <config.h>


namespace component
{
bool operator==(const component::uuid_t& lhs, const component::uuid_t& rhs) {
  return memcmp(&lhs, &rhs, sizeof(component::uuid_t)) == 0;
}

IBase *load_component(const char *dllname, component::uuid_t component_id) {
  return load_component(dllname, component_id, false);
}

/**
 * Called by the client to load the component from a DLL file
 *
 */
IBase *load_component(const char *dllname, component::uuid_t component_id, bool quiet) {

  void *dll = dlopen(dllname, RTLD_NOW);

  if (!dll) {
    const auto dll_path = std::string(COMPONENT_DLL_INSTALL_DIR) + "/" + dllname;

    dll = dlopen(dll_path.c_str(), RTLD_NOW);

    if (!dll) {
      if(!quiet)
        PLOG("Unable to load library (%s) at path (%s) - check dependencies with ldd tool (reason:%s)",
             dllname, dll_path.c_str(), dlerror());
      return nullptr;
    }
  }

  const auto factory_createInstance = reinterpret_cast<void *(*)(component::uuid_t)>(dlsym(dll, "factory_createInstance"));

  char *error;
  if ((error = dlerror()) != nullptr) {
    PLOG("Error: %s\n", error);
    return nullptr;
  }

  IBase *comp = static_cast<IBase *>(factory_createInstance(component_id));

  if (comp == nullptr) {
    throw General_exception("Factory create instance returned nullptr (%s). Possible component ID mismatch.", dllname);
  }

  comp->set_dll_handle(dll); /* record so we can call dlclose() */
  comp->add_ref();

  return comp;
}

void IBase::release_ref() {
  auto val = _ref_count.fetch_sub(1);
  assert(val != 0);
  if (val == 1) {
    this->unload(); /* call virtual unload function */
  }
}

/**
 * Perform pairwise binding of components
 *
 * @param components List of components to bind
 *
 * @return S_OK on success.
 */
status_t bind(std::vector<IBase *> components) {
  for (size_t i = 0; i < components.size(); i++) {
    assert(components[i]);
    for (size_t j = 0; j < components.size(); j++) {
      assert(components[j]);
      if (i == j) continue;
      if (components[i]->bind(components[j]) == -1) {
        PDBG("connect call in pairwise bind failed.");
        return E_FAIL;
      }
    }
  }

  return S_OK;
}
}  // namespace component
