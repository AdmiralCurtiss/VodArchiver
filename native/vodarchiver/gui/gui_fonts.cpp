#include "gui_fonts.h"

#include "imgui.h"

#include "gui_state.h"
#include "vodarchiver/decompress_helper.h"

namespace {
static constexpr char CuprumFontData[] = {
#include "cuprum.h"
};
static constexpr size_t CuprumFontLength = sizeof(CuprumFontData);
static constexpr char NotoSansJpFontData[] = {
#include "noto_sans_jp.h"
};
static constexpr size_t NotoSansJpFontLength = sizeof(NotoSansJpFontData);
} // namespace

namespace VodArchiver {
void LoadFonts(ImGuiIO& io, GuiState& state) {
    static std::optional<std::vector<char>> cuprum =
        SenLib::DecompressFromBuffer(CuprumFontData, CuprumFontLength);
    static std::optional<std::vector<char>> noto =
        SenLib::DecompressFromBuffer(NotoSansJpFontData, NotoSansJpFontLength);

    io.Fonts->Clear();

    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    if (cuprum) {
        io.Fonts->AddFontFromMemoryTTF(cuprum->data(),
                                       static_cast<int>(cuprum->size()),
                                       static_cast<int>(18.0f * state.CurrentDpi),
                                       &config,
                                       io.Fonts->GetGlyphRangesDefault());
        config.MergeMode = true;
    }
    if (noto) {
        io.Fonts->AddFontFromMemoryTTF(noto->data(),
                                       static_cast<int>(noto->size()),
                                       static_cast<int>(20.0f * state.CurrentDpi),
                                       &config,
                                       io.Fonts->GetGlyphRangesJapanese());
    }

    io.Fonts->Build();
}
} // namespace VodArchiver
