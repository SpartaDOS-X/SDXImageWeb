using Microsoft.AspNetCore.Components.Forms;
using Microsoft.AspNetCore.Components.Routing;
using Microsoft.JSInterop;
using MudBlazor;


namespace SDXImageWeb.Pages
{
    public partial class Imager
    {
        SDXRom sdxRom = new SDXRom();
        string fileName;
        bool Loading = false;

        private HashSet<SDXFile> selectedItems = new HashSet<SDXFile>();


        public int PercentUsed
        {
            get
            {
                return
                    sdxRom.Occupied * 100 / sdxRom.Capacity;
            }
        }

        protected override void OnInitialized()
        {
            Navigation.LocationChanged += Navigation_LocationChangedAsync;
        }

        private async Task OnRomUploaded(IBrowserFile file)
        {

            if (file.Size > 1024 * 1024)
            {
                //message = "The chosen file is not a too large";
                //messageType = MessageBarType.Error;
                //onFileError = true;
                return;
            }

            Loading = true;

            //message = null;
            var fileData = new byte[file.Size];
            await file.OpenReadStream(1024*1024).ReadAsync(fileData);
            if (sdxRom.OpenRom(fileData))
            {
                fileName = file.Name;
                //currentCount = sdxRom.FileCount;
                //this.StateHasChanged();
            }

            Loading = false;
        }

        private async void EditConfig()
        {
            var configFile = sdxRom.Files.FirstOrDefault(f => f.Name == "CONFIG  SYS");
            if (configFile != null)
            {
                var content = sdxRom.GetFileText(configFile);
                var parameters = new DialogParameters();
                parameters.Add("ContentText", content);
                parameters.Add("Title", configFile.Name);

                DialogOptions options = new DialogOptions() { CloseButton = true, MaxWidth = MaxWidth.Medium, FullWidth = true };

                var dialog = await DialogService.ShowAsync<SDXImageWeb.Pages.Editor>("Editor", parameters, options);

                var result = await dialog.Result;

                if (!result.Cancelled)
                {
                    var newContent = result.Data.ToString();
                    sdxRom.SetFileText(configFile, newContent);
                }

            }
        }

        private async Task SaveRom()
        {
            if (sdxRom.SaveImage("SDX1.ROM"))
            {
                try
                {
                    var fileStream = new FileStream("SDX1.ROM", FileMode.Open, FileAccess.Read);
                    using var streamRef = new DotNetStreamReference(stream: fileStream);
                    await JS.InvokeVoidAsync("downloadFileFromStream", fileName, streamRef);
                }
                catch (Exception ex)
                {
                    Console.Error.WriteLine($"Cannot open SDX.ROM ! {ex.Message}");
                }
            }
        }

        private void DeleteFiles()
        {
            foreach (var file in selectedItems)
            {
                sdxRom.DeleteFile(file);
            }

            sdxRom.UpdateFileList();
        }

        private async void Navigation_LocationChangedAsync(object? sender, LocationChangedEventArgs e)
        {
            await CloseImage();
        }

        private async Task CloseImage()
        {
            if (sdxRom.Modified)
            {

                bool? result = await DialogService.ShowMessageBox(
         $"Warning",
         "Do you want to save the changes ?",
         yesText: "Save", cancelText: "Cancel");

                if (result != null)
                {
                    await SaveRom();
                }
            }

            sdxRom.Close();
        }

        private async Task OnFileUploaded(IBrowserFile file)
        {          
            sdxRom.UpdateFileList();

        }

    }
}