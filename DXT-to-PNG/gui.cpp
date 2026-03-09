#include "gui.h"
#include "converter.h"
#include "imgui/imgui.h"
#include <ShlObj.h>
#include <shellapi.h>
#include <format>
#include <thread>

// ─── File / folder dialogs ────────────────────────────────────────────────────

static void OpenFileDialog(AppState& state) {
    IFileOpenDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
        IID_IFileOpenDialog, reinterpret_cast<void**>(&dlg)))) return;

    const COMDLG_FILTERSPEC filter = { L"DXT Textures", L"*.dxt" };
    dlg->SetFileTypes(1, &filter);
    dlg->SetOptions(FOS_ALLOWMULTISELECT | FOS_FILEMUSTEXIST);

    if (SUCCEEDED(dlg->Show(nullptr))) {
        IShellItemArray* items = nullptr;
        if (SUCCEEDED(dlg->GetResults(&items))) {
            DWORD count = 0;
            items->GetCount(&count);
            std::lock_guard lock(state.mtx);
            for (DWORD i = 0; i < count; i++) {
                IShellItem* item = nullptr;
                if (SUCCEEDED(items->GetItemAt(i, &item))) {
                    PWSTR path = nullptr;
                    if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                        state.files.push_back({ std::filesystem::path(path) });
                        CoTaskMemFree(path);
                    }
                    item->Release();
                }
            }
            items->Release();
        }
    }
    dlg->Release();
}

static void OpenFolderDialog(AppState& state) {
    IFileOpenDialog* dlg = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,
        IID_IFileOpenDialog, reinterpret_cast<void**>(&dlg)))) return;

    dlg->SetOptions(FOS_PICKFOLDERS | FOS_PATHMUSTEXIST);

    if (SUCCEEDED(dlg->Show(nullptr))) {
        IShellItem* item = nullptr;
        if (SUCCEEDED(dlg->GetResult(&item))) {
            PWSTR path = nullptr;
            if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                std::lock_guard lock(state.mtx);
                state.outputDir = path;
                CoTaskMemFree(path);
            }
            item->Release();
        }
    }
    dlg->Release();
}

// ─── Drag-and-drop ────────────────────────────────────────────────────────────

void HandleDropFiles(HDROP drop, AppState& state) {
    const UINT count = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
    std::lock_guard lock(state.mtx);
    for (UINT i = 0; i < count; i++) {
        WCHAR path[MAX_PATH];
        DragQueryFileW(drop, i, path, MAX_PATH);
        state.files.push_back({ std::filesystem::path(path) });
    }
    DragFinish(drop);
}

// ─── Conversion dispatch ──────────────────────────────────────────────────────

static void StartConversion(AppState& state) {
    // Phase 1: under lock, mark all Pending entries as Converting and fix output paths.
    struct Job { size_t index; std::filesystem::path input, output; };
    std::vector<Job> jobs;

    {
        std::lock_guard lock(state.mtx);
        for (size_t i = 0; i < state.files.size(); i++) {
            FileEntry& f = state.files[i];
            if (f.status != FileStatus::Pending) continue;

            const std::filesystem::path out = state.outputDir.empty()
                ? f.input.parent_path() / (f.input.stem().string() + ".png")
                : state.outputDir / (f.input.stem().string() + ".png");

            f.output = out;
            f.status = FileStatus::Converting;
            jobs.push_back({ i, f.input, out });
        }
        if (jobs.empty()) return;
        state.doneCount  = 0;
        state.totalCount = static_cast<int>(jobs.size());
        state.converting = true;
    }

    // Phase 2: launch one detached thread per job.
    for (const Job& job : jobs) {
        std::thread([&state, job]() {
            auto result = Convert(job.input, job.output);
            {
                std::lock_guard lock(state.mtx);
                FileEntry& f = state.files[job.index];
                if (result) {
                    f.status = FileStatus::Done;
                } else {
                    f.status = FileStatus::Error;
                    f.error  = result.error();
                }
            }
            if (++state.doneCount >= state.totalCount)
                state.converting = false;
        }).detach();
    }
}

// ─── ImGui rendering ─────────────────────────────────────────────────────────

static constexpr const char* StatusLabel(FileStatus s) {
    switch (s) {
    case FileStatus::Pending:    return "Pending";
    case FileStatus::Converting: return "Converting...";
    case FileStatus::Done:       return "Done";
    case FileStatus::Error:      return "Error";
    }
    return "";
}

void RenderGui(AppState& state) {
    const ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);
    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoTitleBar  | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove      | ImGuiWindowFlags_NoSavedSettings);

    const bool busy = state.converting.load();

    // ── Toolbar ──
    if (busy) ImGui::BeginDisabled();
    if (ImGui::Button("Add Files...")) OpenFileDialog(state);
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        std::lock_guard lock(state.mtx);
        state.files.clear();
    }
    if (busy) ImGui::EndDisabled();

    // ── Output folder row ──
    ImGui::Separator();
    ImGui::AlignTextToFramePadding();
    ImGui::Text("Output folder:");
    ImGui::SameLine();
    {
        std::lock_guard lock(state.mtx);
        ImGui::TextUnformatted(state.outputDir.empty()
            ? "(same as source)"
            : state.outputDir.string().c_str());
    }
    ImGui::SameLine();
    if (!busy && ImGui::Button("Browse...")) OpenFolderDialog(state);

    ImGui::Separator();

    // ── File list table ──
    const float listHeight = io.DisplaySize.y - 140.0f;
    if (ImGui::BeginTable("##files", 3,
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp,
        ImVec2(0.0f, listHeight)))
    {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("File",   ImGuiTableColumnFlags_WidthStretch, 3.0f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Error",  ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableHeadersRow();

        std::lock_guard lock(state.mtx);
        for (const FileEntry& f : state.files) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(f.input.filename().string().c_str());
            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(StatusLabel(f.status));
            ImGui::TableSetColumnIndex(2);
            if (f.status == FileStatus::Error)
                ImGui::TextUnformatted(f.error.c_str());
        }
        ImGui::EndTable();
    }

    // ── Bottom bar ──
    ImGui::Separator();

    const int done  = state.doneCount.load();
    const int total = state.totalCount.load();
    if (total > 0)
        ImGui::ProgressBar(static_cast<float>(done) / static_cast<float>(total),
            ImVec2(-1.0f, 0.0f), std::format("{} / {}", done, total).c_str());

    bool hasPending = false;
    if (!busy) {
        std::lock_guard lock(state.mtx);
        for (const FileEntry& f : state.files)
            if (f.status == FileStatus::Pending) { hasPending = true; break; }
    }

    if (busy || !hasPending) ImGui::BeginDisabled();
    if (ImGui::Button(busy ? "Converting..." : "Convert All", ImVec2(-1.0f, 0.0f)))
        StartConversion(state);
    if (busy || !hasPending) ImGui::EndDisabled();

    ImGui::End();
}
