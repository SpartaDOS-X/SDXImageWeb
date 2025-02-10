using Microsoft.AspNetCore.Components.Forms;
using Microsoft.AspNetCore.Components.Web;
using Microsoft.AspNetCore.Components.WebAssembly.Http;
using Microsoft.AspNetCore.Components;
using Microsoft.JSInterop;
using MudBlazor;

namespace SDXImageWeb.Pages.Components
{
    public partial class SaveDialog
    {
        [CascadingParameter] MudDialogInstance? MudDialog { get; set; }

        [Parameter] public string Url { get; set; } = string.Empty;

        [Parameter] public string UrlText { get; set; } = string.Empty;

        [Parameter] public string Title { get; set; } = "Text";

        private void Cancel()
        {
            MudDialog?.Cancel();
        }

        private void Save()
        {
            MudDialog?.Close(DialogResult.Ok(UrlText));
        }
  
    }

}
