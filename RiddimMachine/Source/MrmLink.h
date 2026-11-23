// Link de harmonia do Mastor Riddim Machine MRM-0420 com o Mastor Tune MT-0420.
// O MRM publica (20 vezes por segundo) a harmonia e o acorde que está tocando num arquivo pequeno
// mapeado em memória, na pasta temporária do sistema. O Mastor Tune lê o mesmo arquivo: funciona
// com os dois plugins na mesma DAW (no mesmo processo ou não) e não precisa de roteamento.
// Usado por RiddimMachine/Source/MrmProcessor.cpp (escreve) e Tune/Source/PluginProcessor.cpp (lê).
#pragma once
#include <juce_core/juce_core.h>
#include <atomic>
#include <cstring>

namespace mrmlink
{
static constexpr juce::uint32 kMagic = 0x4d524d31;   // "MRM1"

struct Data
{
    juce::uint32 magic;
    juce::uint32 seq;          // par = pronto; ímpar = escrevendo
    juce::int64  stampMs;      // Time::currentTimeMillis() da última escrita
    juce::int32  playing;      // 1 = o riddim está tocando
    juce::int32  curSlot;      // acorde tocando agora (0..3) ou -1
    juce::int32  numSlots;     // 4
    juce::int32  root[4];      // 0 = C .. 11 = B
    juce::int32  minor[4];     // 1 = menor
    juce::int32  on[4];        // acorde ligado
    float        beats[4];     // duração de cada acorde em tempos
    float        bpm;
    float        posBeats;     // posição dentro do loop da harmonia (-1 parado)
};

inline juce::File linkFile()
{
    return juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("DubStation-MRM-0420-Link.bin");
}

// abre (e cria do tamanho certo) o arquivo do link
inline std::unique_ptr<juce::MemoryMappedFile> open (bool create)
{
    auto f = linkFile();
    if (! f.existsAsFile())
    {
        if (! create) return {};
        juce::MemoryBlock zero (sizeof (Data), true);
        if (! f.replaceWithData (zero.getData(), zero.getSize())) return {};
    }
    else if (f.getSize() < (juce::int64) sizeof (Data))
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
            if (++retry % 40 != 1) return;   // tenta de novo a cada ~2 s
            mm = open (true);
            if (mm == nullptr) return;
        }
        auto* p = static_cast<Data*> (mm->getData());
        d.magic = kMagic;
        d.stampMs = juce::Time::currentTimeMillis();
        const juce::uint32 s = p->seq;
        p->seq = s | 1u;
        std::atomic_thread_fence (std::memory_order_release);
        const juce::uint32 nextSeq = (s | 1u) + 1u;
        d.seq = s | 1u;
        std::memcpy (p, &d, sizeof (Data));
        std::atomic_thread_fence (std::memory_order_release);
        p->seq = nextSeq;
    }
private:
    std::unique_ptr<juce::MemoryMappedFile> mm;
    int retry = 0;
};

class Reader
{
public:
    // true = o MRM-0420 está aberto e publicou há menos de 1,5 s
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
} // namespace mrmlink
