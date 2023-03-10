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

        bool Loading { get; set; }

        private HashSet<SDXFile> selectedItems = new HashSet<SDXFile>();

        public double[] data = { 0, 0 };
        public string[] labels = { "Free", "Occupied" };

        string searchName = string.Empty;

        bool IsSaved { get; set; }

        private MudTable<SDXFile>? mudTable;

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
                Snackbar.Add("The file is too large (> 1024kB)", Severity.Error);
                return;
            }

            var fileData = new byte[file.Size];

            Loading = true;

            fileName = file.Name;

            await file.OpenReadStream(1024 * 1024).ReadAsync(fileData);
            await Task.Delay(1);

            //message = null;
            if (sdxRom.OpenRom(fileData))
            {
                //currentCount = sdxRom.FileCount;
                //this.StateHasChanged();
                UpdateImageInfo(false);

                IsSaved = false;
            }
            else
            {
                Snackbar.Add("Cannot open the file. Is this a valid SDX ROM ?", Severity.Error);
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

                    if(!newContent.EndsWith((char)0x0a))
                        newContent += (char)0x0a;

                    if (sdxRom.SetFileText(configFile, newContent))
                    {
                        Snackbar.Add($"{configFile.FileName} updated", Severity.Success);
                        StateHasChanged();
                    }
                    else
                    {
                        Snackbar.Add($"Cannot update {configFile.FileName}", Severity.Error);
                    }
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
                    IsSaved = true;

                }
                catch (Exception ex)
                {
                    Snackbar.Add($"Cannot save the image", Severity.Error);
                    Console.Error.WriteLine($"Cannot open SDX.ROM ! {ex.Message}");
                }
            }
        }

        private void DeleteFiles()
        {
            bool error = false;
            foreach (var file in selectedItems)
            {
                if (!sdxRom.DeleteFile(file))
                {
                    Snackbar.Add($"Cannot update {file.FileName}", Severity.Error);
                    error = true;
                }
            }
            
            UpdateImageInfo();

            if (!error)
            {
                if (selectedItems.Count > 1)
                    Snackbar.Add($"{selectedItems.Count} files deleted", Severity.Success);
                else
                    Snackbar.Add($"{selectedItems.FirstOrDefault()?.FileName} deleted", Severity.Success);
            }

        }

        private void UpdateImageInfo(bool reloadFiles = true)
        {
            if (reloadFiles)
                sdxRom.UpdateFileList();
            data[0] = sdxRom.FreeSpace;
            data[1] = sdxRom.Occupied;
        }

        private async void Navigation_LocationChangedAsync(object? sender, LocationChangedEventArgs e)
        {
            await CloseImage();
        }

        private async Task CloseImage()
        {
            if (sdxRom.Valid && sdxRom.Modified && !IsSaved)
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

            Navigation.LocationChanged -= Navigation_LocationChangedAsync;
        }

        private void OnFileUploaded(IBrowserFile file)
        {
            UpdateImageInfo();
        }

        private bool FilterName(SDXFile element)
        {
            if (string.IsNullOrWhiteSpace(searchName))
                return true;
            if (element.Name.Contains(searchName, StringComparison.OrdinalIgnoreCase))
                return true;
            return false;
        }

    }
}