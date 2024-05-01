#ifndef _LANGUAGE_LANG_HPP
#define _LANGUAGE_LANG_HPP

// Include Italian language
#if FABLAB_LANG_IT_IT
#include "language/it-IT.hpp"
namespace fablabbg::strings
{
  using namespace fablabbg::strings::it_IT;
}
#endif

// Include English language
#ifdef FABLAB_LANG_EN_US
#include "language/en-US.hpp"
namespace fablabbg::strings
{
  using namespace fablabbg::strings::en_US;
}
#endif

// Repeat the pattern above if other languages are needed
#endif