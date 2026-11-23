// Link do Mastor Key Scout MKS-0420 com o Mastor Tune MT-0420.
// O Key Scout fica no master (ou no grupo da base), escuta o riddim e publica 20 vezes por segundo
// o tom e o acorde do momento num arquivo pequeno mapeado em memória, na pasta temporária do sistema.
// O Mastor Tune lê o mesmo arquivo: sem sidechain e sem roteamento, em qualquer DAW.
// Usado por KeyScout/Source/PluginProcessor.cpp (escreve) e Tune/Source/PluginProcessor.cpp (lê).
#pragma once
#include <juce_core/juce_core.h>
#include <atomic>
#include <cstring>

namespace scoutlink
{
static constexpr juce::uint32 kMagic = 0x4d4b5331;   // "MKS1"

struct Data
{
    juce::uint32 magic;
    juce::uint32 seq;          // par = pronto; ímpar = escrevendo
    juce::int64  stampMs;      // Time::currentTimeMillis() da última escrita
    juce::int32  signal;       // 1 = tem riddim tocando agora
    juce::int32  keyRoot;      // tom aprendido (0 = C .. 11 = B) ou -1
    juce::int32  keyMinor;     // 1 = menor
    juce::int32  chordRoot;    // acorde do momento ou -1
    juce::int32  chordMinor;
    juce::int32  hold;         // 1 = tom travado pelo usuário
    float        chroma[12];   // cromagrama curto (0..1), para quem quiser desenhar
};

inline juce::File linkFile()
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("DubStation-MKS-0420-Link.bin");
}

inline std::unique_ptr<juce::MemoryMappedFile> open (bool create)
{
    auto f = linkFile();
    if (! f.existsAsFile() || f.getSize() < (juce::int64) sizeof (Data))
    {
        if (! create) return {};
        juce::MemoryBlock zero (sizeof (Data), true);
        if (! f.replaceWithData (zero.getData(), zero.getSize())) return {};
    }
    // os dois lados abrem em leitura e escrita (no Windows é o que deixa compartilhar o arquivo)
    auto mm = std::make_unique<juce::MemoryMappedFile> (f, juce::MemoryMappedFile::readWrite, false);
    if (mm->getData() == nullptr || mm->getSize() < sizeof (Data)) return {};
    return mm;
}

class Writer
{
public:
    void write (Data d)
    {
        if (mm == nullptr)
        {
            if (++retry % 40 != 1) return;
            mm = open (true);
            if (mm == nullptr) return;
        }
        auto* p = static_cast<Data*> (mm->getData());
        d.magic = kMagic;
        d.stampMs = juce::Time::currentTimeMillis();
        const juce::uint32 s = p->seq;
        p->seq = s | 1u;
        std::atomic_thread_fence (std::memory_order_release);
        d.seq = s | 1u;
        std::memcpy (p, &d, sizeof (Data));
        std::atomic_thread_fence (std::memory_order_release);
        p->seq = (s | 1u) + 1u;
    }
private:
    std::unique_ptr<juce::MemoryMappedFile> mm;
    int retry = 0;
};

class Reader
{
public:
    // true = um Key Scout está aberto e publicou há menos de 1,5 s
    bool read (Data& out)
    {
        if (mm == nullptr)
        {
            if (++retry % 40 != 1) return false;
            mm = open (false);
            if (mm == nullptr) return false;
        }
        auto* p = static_cast<const Data*> (mm->getData());
        for (int attempt = 0; attempt < 4; ++attempt)
        {
            const juce::uint32 s1 = p->seq;
            std::atomic_thread_fence (std::memory_order_acquire);
            if (s1 & 1u) continue;
            Data d;
            std::memcpy (&d, p, sizeof (Data));
            std::atomic_thread_fence (std::memory_order_acquire);
            if (p->seq != s1) continue;
            if (d.magic != kMagic) return false;
            if (std::abs (juce::Time::currentTimeMillis() - d.stampMs) > 1500) return false;
            out = d;
            return true;
        }
        return false;
    }
private:
    std::unique_ptr<juce::MemoryMappedFile> mm;
    int retry = 0;
};
} // namespace scoutlink
