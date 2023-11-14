using Microsoft.JSInterop;
using System;
using System.Reflection.Metadata;
using System.Runtime.InteropServices;
using System.Text;

namespace SDXImageWeb
{
    public class SDXRom
    {

        [DllImport("SDXImgTool")]
        static extern IntPtr CreateCart();

        [DllImport("SDXImgTool")]
        static extern bool OpenRom(IntPtr cart, byte[] data, int length);

        [DllImport("SDXImgTool")]
        static extern int GetFileCount(IntPtr cart);

        [DllImport("SDXImgTool")]
        static extern IntPtr GetFileName(IntPtr cart, int index);

        [DllImport("SDXImgTool")]
        static extern int GetFileSize(IntPtr cart, int index);
        
        [DllImport("SDXImgTool")]
        static extern IntPtr GetFileContents(IntPtr cart, int index);

        [DllImport("SDXImgTool")]
        static extern bool SetFileContents(IntPtr cart, int index, byte[] contents, int length);

        [DllImport("SDXImgTool")]
        static extern int GetTotalBytes(IntPtr cart);

        [DllImport("SDXImgTool")]
        static extern int GetCapacity(IntPtr cart);

        [DllImport("SDXImgTool", CharSet = CharSet.Ansi)]
        static extern bool SaveImage(IntPtr cart, string fileName);

        [DllImport("SDXImgTool", CharSet = CharSet.Ansi)]
        static extern bool RemoveFile(IntPtr cart, string fileName);

        [DllImport("SDXImgTool")]
        static extern void CloseCart(IntPtr cart);

        [DllImport("SDXImgTool")]
        static extern bool GetCarEntry(IntPtr cart, string filename, ref CCAREntry info);

        [DllImport("SDXImgTool")]
        static extern int GetType(IntPtr cart);

        [DllImport("SDXImgTool")]
        static extern bool InsertFile(IntPtr cart, string filename, byte[] contents, int length);

        IntPtr cart = IntPtr.Zero;

        List<SDXFile> files = new List<SDXFile>();
        public List<SDXFile> Files {  get { return files; } }   

        public SDXRom()
        {
            cart = CreateCart();
        }

        public bool Modified { get; private set; } = false;

        public bool Valid { get; private set; } = false;

        public bool HasConfig { get { return files.Any(f => f.Name.Equals("CONFIG  SYS")); } }

        internal int signatureLength = 0;
        internal byte[] signature;


        public bool OpenRom(byte[] data)
        {
            var carSignature = System.Text.Encoding.Default.GetString(data,0,4);

            signatureLength = 0;
            if (carSignature.Equals("CART"))
            {
                signatureLength = 16;                    
            }

            signature = new byte[signatureLength];
            Array.Copy(data, signature, signatureLength);

            bool res = OpenRom(cart, data.Skip(signatureLength).ToArray(), data.Length - signatureLength);
            if (res)
            {
                UpdateFileList();
                Modified = false;
                Valid = true;
            }  

            return res;
        }

        public int FileCount
        {
            get { 
                if (cart != IntPtr.Zero)
                    return GetFileCount(cart);
                return 0;
            }    
        }

        public int Capacity
        {
            get
            {
                if (cart != IntPtr.Zero)
                    return GetCapacity(cart);
                return 0;
            }
        }

        public int Occupied
        {
            get
            {
                if (cart != IntPtr.Zero)
                    return GetTotalBytes(cart);
                return 0;
            }
        }

        public int FreeSpace
        {
            get
            {
                return Capacity - Occupied;
            }
        }

        public string Type
        {
            get
            {
                //#define CART_NORMAL			0
                //#define CART_MAXFLASH1MB	1
                //#define CART_SDX128         2
                //#define CART_MAXFLASH8MB	3
                //#define CART_SDX256         4

                int type = GetType(cart);

                switch (type)
                {
                    case 0: // CART_NORMAL:
                        return "SpartaDOS X 4.2x";
                    case 1: // CART_MAXFLASH1MB:
                        return "Maxflash/Turbo Freezer/SicCart/Ultimate/SIDE";
                    case 2: // CART_SDX128:
                        return "SpartaDOS X / intSDX";
                    /*
                        case CART_MAXFLASH8MB:
                            sitem = "Maxflash 8MBit (1MB)";
                            break;
                        case CART_SDX256:
                            sitem = "SpartaDOS X 256kB (intSDX 256)";
                            break;
                    */
                    default:
                        return type.ToString();
                }
            }

        }

        private string? GetFileName(int index)
        {
            IntPtr ptr = GetFileName(cart, index);
            var name = Marshal.PtrToStringAnsi(ptr);
            return name;
        }

        public int GetFileSize(int index)
        {
            return GetFileSize(cart, index);
        }

        private byte[] GetFileContents(int index)
        {
            IntPtr ptr = GetFileContents(cart, index);

            int length = GetFileSize(index);
            byte[] contents = new byte[length];

            Marshal.Copy(ptr, contents, 0, length);
            return contents;
        }

        public byte[] GetFileContents(SDXFile file)
        {
            return GetFileContents(file.Id);
        }

        public string GetFileText(SDXFile file)
        {
            var content = this.GetFileContents(file.Id);
            for (var i = 0; i < content.Length; i++)
            {
                if (content[i] == 0x9b)
                {
                    content[i] = 0x0a;
                }
            }

            return System.Text.Encoding.ASCII.GetString(content);
        }

        public bool SetFileText(SDXFile file, string text)
        {
            var content = System.Text.Encoding.ASCII.GetBytes(text);
            for (var i = 0; i < content.Length; i++)
            {
                if (content[i] == 0x0a)
                {
                    content[i] = 0x9b;
                }
            }

            Modified = true;

            return SetFileContents(cart, file.Id, content, content.Length);
        }


        public void UpdateFileList()
        {
            files.Clear();

            for(int i=0; i<FileCount; i++)
            {
                var name = GetFileName(i);
                var size = GetFileSize(i);

                if (!string.IsNullOrEmpty(name))
                {
                    var file = new SDXFile(i, name, size);
                    CCAREntry info = new CCAREntry();
                    if (GetCarEntry(cart, name, ref info))
                    //if (carEntry != IntPtr.Zero)
                    {
                        file.CarEntry = info; // (CCAREntry)Marshal.PtrToStructure(carEntry, typeof(CCAREntry));                        
                    }
                    files.Add(file);
                }
            }      
        }

        public bool SaveImage(string filename)
        {
            if (signature.Length == 0)
            {
                return SaveImage(cart, filename);
            }
            else
            {
                if (SaveImage(cart, "sdx.tmp"))
                {
                    try
                    {
                        var imageStream = new FileStream("sdx.tmp", FileMode.Open, FileAccess.Read);
                        var fileStream = new FileStream(filename, FileMode.Create, FileAccess.Write);
                        fileStream.Write(signature, 0, signature.Length);
                        imageStream.CopyTo(fileStream);
                        return true;
                    }
                    catch
                    {
                        Console.WriteLine("Cannot create .CAR output !");
                    }

                }
            }

            return false;

        }

        internal bool DeleteFile(SDXFile file)
        {
            Modified = true;

            return RemoveFile(cart, file.Name);
        }


        internal bool InsertFile(string filename, byte[] data)
        {
            bool res = InsertFile(cart, filename, data, data.Length);

            if (res)
                Modified = true;

            return res;
        }

        public void Close()
        {
            CloseCart(cart);
            Files.Clear();
            Valid = false;
            Modified = false;
        }
    }

    public class SDXFile
    {
        public int Id { get; set; }

        public string Name { get; set; }

        public int Size { get; set; }

        internal CCAREntry CarEntry { get; set; }

        public string Date
        {
            get
            {
                return $"{CarEntry.date0:D2}-{CarEntry.date1:D2}-{CarEntry.date2:D2}";
            }
        }

        public string DateYMD
        {
            get
            {
                return $"{CarEntry.date2:D2}-{CarEntry.date1:D2}-{CarEntry.date0:D2}";
            }
        }

        public string Time
        {
            get
            {
                return $"{CarEntry.time0:D2}:{CarEntry.time1:D2}:{CarEntry.time2:D2}";
            }
        }

        /// <summary>
        /// DOS file name (with dot between name and extension)
        /// </summary>
        public string FileName
        {
            get
            {
                StringBuilder sb = new StringBuilder();

                sb.Append(Name.Substring(0,8).Trim());
                sb.Append('.');
                sb.Append(Name.Substring(8,3).Trim());

                return sb.ToString();
            }
        }

        public string Status
        {
            get
            {
                string status = string.Empty;

                for (int i = 0; i < 8; i++)
                {
                    if ((CarEntry.status & (1 << i)) != 0)
                        status += "pha dsxo"[i];
                }

                return status;
            }
        }

        public SDXFile(int id, string name, int size)
        {
            Id = id;
            Name = name;
            Size = size;
        }     

    }

    [StructLayout(LayoutKind.Sequential)]
    struct CCAREntry
    {
	    int directory_position = 0;
        public byte status = 0;
        public byte time0 = 0;
        public byte time1 = 0;
        public byte time2 = 0;
        public byte date0 = 0;
        public byte date1 = 0;
        public byte date2 = 0;

        public CCAREntry()
        {
        }
    }

}
