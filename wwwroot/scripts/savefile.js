// see: https://web.dev/patterns/files/save-a-file


export const downloadFileFromStream = async (fileName, contentStreamReference) => {
    const arrayBuffer = await contentStreamReference.arrayBuffer();
    const blob = new Blob([arrayBuffer]);
    const url = URL.createObjectURL(blob);
    const anchorElement = document.createElement('a');
    anchorElement.href = url;
    anchorElement.download = fileName ?? '';
    anchorElement.click();
    anchorElement.remove();
    URL.revokeObjectURL(url);
}

// Feature detection. The API needs to be supported and the app not run in an iframe.
export const supportsFileSystemAccess = () => {
    return 'showSaveFilePicker' in window && window.self === window.top;
};


export const saveFileContents = async (contentStreamReference, suggestedName) => {
  
    const arrayBuffer = await contentStreamReference.arrayBuffer();
    const blob = new Blob([arrayBuffer]);

    // If the File System Access API is supported�
    if (supportsFileSystemAccess()) {
        try {
            // Show the file save dialog.
            const handle = await showSaveFilePicker({
                suggestedName,
            });
            // Write the blob to the file.
            const writable = await handle.createWritable();
            await writable.write(blob);
            await writable.close();
            return;
        } catch (err) {
            // Fail silently if the user has simply canceled the dialog.
            if (err.name !== 'AbortError') {
                console.error(err.name, err.message);
                return;
            }
        }
    }
    else {

        const url = URL.createObjectURL(blob);
        clickDownloadAnchor(url);
        URL.revokeObjectURL(url);
    }
};

export const createDownloadLink = async (contentStreamReference) => {
    const arrayBuffer = await contentStreamReference.arrayBuffer();
    const blob = new Blob([arrayBuffer], { type: 'octet/stream' });

    return URL.createObjectURL(blob);
};

export const clickDownloadAnchor = async (url, suggestedName) => {
    const anchorElement = document.createElement('a');
    anchorElement.href = url;
    anchorElement.download = suggestedName ?? '';
    anchorElement.style.display = 'none';
    document.body.append(anchorElement);
    anchorElement.click();
    anchorElement.remove();
};