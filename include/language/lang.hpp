#ifndef _FABOMATIC_LANG_HPP
#define _FABOMATIC_LANG_HPP

// Include Italian language
#if FABOMATIC_LANG_IT_IT
#include "language/it-IT.hpp"
namespace fabomatic::strings
{
  using namespace fabomatic::strings::it_IT;
}
#endif

// Include English language
#ifdef FABOMATIC_LANG_EN_US
#include "language/en-US.hpp"
namespace fabomatic::strings
{
  using namespace fabomatic::strings::en_US;
}
#endif

// Repeat the pattern above if other languages are needed

#endif // _FABOMATIC_LANG_HPP