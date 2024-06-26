#include <filesystem>
#include <fstream>

#include <wrl.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <d3d12.h>
#include <pix.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <memory>
#include <vector>
#include <agile.h>
#include <concrt.h>

#include "i_interface.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Core;
using namespace Windows::UI::Input;
using namespace Windows::System;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

std::string PlatformToStd(Platform::String^ inStr) {
	std::wstring str2(inStr->Begin());
	std::string str3(str2.begin(), str2.end());
	return str3;
}

Platform::String^ StdToPlatform(std::string inStr) {
	std::wstring wid_str = std::wstring(inStr.begin(), inStr.end());
	const wchar_t* w_char = wid_str.c_str();
	return ref new Platform::String(w_char);
}

FString uwp_GetCWD()
{
    char maxPath[MAX_PATH];
    GetCurrentDirectoryA(sizeof(maxPath), maxPath);
    return maxPath;
}

std::string PickAFolder();

FString uwp_GetAppDataPath()
{
    std::string local_state = PlatformToStd(Windows::Storage::ApplicationData::Current->LocalFolder->Path);
    std::error_code error_code;
    if (std::filesystem::exists(local_state, error_code) && 
        std::filesystem::is_directory(local_state, error_code) && 
        !error_code)
    {
        std::filesystem::path app_data_redirect_file = local_state; app_data_redirect_file /= "redirect_appdata.txt";
        if (!std::filesystem::exists(app_data_redirect_file, error_code) ||
            !std::filesystem::is_regular_file(app_data_redirect_file, error_code) ||
            error_code)
        {
            std::filesystem::path new_path = PickAFolder();
            if (std::filesystem::exists(new_path, error_code) &&
                std::filesystem::is_directory(new_path, error_code) &&
                !error_code)
            {
                std::ofstream redirect_file_stream(app_data_redirect_file.string(), std::ios::trunc);
                redirect_file_stream << new_path.string() << std::endl;
            }
            else {
                std::ofstream redirect_file_stream(app_data_redirect_file.string(), std::ios::trunc);
                redirect_file_stream << local_state << std::endl;
            }
        }
        std::ifstream redirect_file_stream(app_data_redirect_file.string());
        std::string appdata;
        if (std::getline(redirect_file_stream, appdata) &&
            std::filesystem::exists(appdata, error_code) && 
            !error_code)
        {
            return appdata.c_str();
        }
    }

    return PlatformToStd(Windows::Storage::ApplicationData::Current->LocalFolder->Path).c_str();
}

unsigned int GenerateRandomNumber()
{
    // Generate a random number.
    unsigned int random = Windows::Security::Cryptography::CryptographicBuffer::GenerateRandomNumber();
    return random;
}

unsigned int uwp_MakeRNGSeed()
{
    return GenerateRandomNumber();
}

void RedrawProgressBar(int CurPos, int MaxPos)
{
    //todo
}
void CleanProgressBar()
{
    //todo
}

template <typename T>
void WaitForAsync(IAsyncOperation<T>^ A)
{
    while (A->Status == Windows::Foundation::AsyncStatus::Started) {
        CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
    }

    Windows::Foundation::AsyncStatus S = A->Status;
}

std::string PickAFolder()
{
    std::string out = "";
    auto folderPicker = ref new Windows::Storage::Pickers::FolderPicker();
    folderPicker->SuggestedStartLocation = Windows::Storage::Pickers::PickerLocationId::Desktop;
    folderPicker->FileTypeFilter->Append("*");

    auto folder_operation = folderPicker->PickSingleFolderAsync();
    WaitForAsync(folder_operation);
    auto folder = folder_operation->GetResults();
    if (folder != nullptr) {
        // Application now has read/write access to all contents in the picked file
        Windows::Storage::AccessCache::StorageApplicationPermissions::FutureAccessList->AddOrReplace("PickedFolderToken", folder);
        auto selected = folder->Path;
        out = std::string(selected->Begin(), selected->End());
    }
    return out;
}

#define MAX_MENU_ENTRIES 6

int uwp_ChooseWad(WadStuff* wads, int numwads, int defaultiwad, int& autoloadflags)
{
    int selected = defaultiwad;
    Windows::UI::Popups::PopupMenu^ popupmenu = ref new Windows::UI::Popups::PopupMenu();

    int current_base = 0;
    Windows::UI::Popups::IUICommand^ result = nullptr;

    int displacement = min(numwads, MAX_MENU_ENTRIES);
    while (result == nullptr)
    {
        popupmenu->Commands->Clear();
        for (int i = 0; i < displacement; ++i)
        {
            popupmenu->Commands->Append(
                ref new Windows::UI::Popups::UICommand(
                    StdToPlatform(wads[(current_base + i) % numwads].Name.GetChars()),
                    ref new Windows::UI::Popups::UICommandInvokedHandler([&selected, i, current_base, numwads](Windows::UI::Popups::IUICommand^ command)
                        {
                            selected = (current_base + i) % numwads;
                        }
                    )
                )
            );
        }
        auto asyncop = popupmenu->ShowForSelectionAsync(CoreWindow::GetForCurrentThread()->Bounds);
        WaitForAsync(asyncop);

        if (numwads > MAX_MENU_ENTRIES)
            current_base = (current_base + MAX_MENU_ENTRIES) % numwads;
        result = asyncop->GetResults();
    }

    return selected;
}