#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

namespace
{
    struct Fonts
    {
        Typeface::Ptr title  = Typeface::createSystemTypefaceFor (BinaryData::ArchivoBlackRegular_ttf, (size_t) BinaryData::ArchivoBlackRegular_ttfSize);
    };
    Fonts& fonts() { static SharedResourcePointer<Fonts> f; return *f; }
}

Font mf::title (float h)  { return Font (FontOptions (fonts().title).withHeight (h)); }
