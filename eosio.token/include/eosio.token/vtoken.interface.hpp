#pragma once

#include "vtoken.common.hpp"

namespace vtoken {

   class interface : public vtoken_base {
      public:
         using vtoken_base::vtoken_base;

         bool allow( account_name acnt );
   };

}