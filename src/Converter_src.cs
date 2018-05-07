using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Chip8ROMConverter
{
	class Program
	{
		static void Main(string[] args)
		{
			if (args.Length > 0) {
				byte[] romData_Raw = File.ReadAllBytes(args[0]);
				byte[] romData = new byte[romData_Raw.Length + 6 + 16];
				//Chip84
				romData[0] = 0x43;
				romData[1] = 0x68;
				romData[2] = 0x69;
				romData[3] = 0x70;
				romData[4] = 0x38;
				romData[5] = 0x34;
				//Control map
				for (int i = 0; i < 16; i++) {
					romData[6 + i] = (byte)i;
				}

				Array.Copy(romData_Raw, 0, romData, 6 + 16, romData_Raw.Length);
				string fileName = Path.GetFileNameWithoutExtension(args[0]);

				int fileSize, sum = 0;
				int[] buff = new int[0xfff];
				byte[] checksum = new byte[2];
				byte[] header = {
					0x2A, 0x2A, 0x54, 0x49, 0x38, 0x33, 0x46, 0x2A,		// **TI83F*
					0x1A, 0x0A, 0x00,									// signature

					0x43, 0x68, 0x69, 0x70, 0x2D, 0x38, 0x34, 0x20,		// comment area
					0x52, 0x4F, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
					0x00, 0x00,

					0x00, 0x00 //data size
				};

				//Max fileSize: 65516 bytes
				byte[] varheader = {
					0x0D,0x00,											// start of variable header
					0x00,0x00,											// length of variable in bytes
					0x15,												// variable type ID. 0x15 is for appvar
					0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,			// variable name (max 8 characters)
					0x00,												// version
					0x00,												// flag. 80h for archived variable. 00h otherwise
					0x00,0x00,											// length of variable in bytes (copy)
					0x00,0x00											// length of variable in bytes (another copy...)
				};

				fileSize = romData.Length;

				if (fileSize > 0xFFEC) {
					System.Environment.Exit(1);
				}

				header[53] = (byte)((fileSize + varheader.Length) & 0xff);
				header[54] = (byte)(((fileSize + varheader.Length)>>8) & 0xff);
				varheader[2] = (byte)((fileSize + 2) & 0xff);
				varheader[3] = (byte)(((fileSize + 2)>>8) & 0xff);
				for (int i = 0; i < 8; i++) {
					if (i >= fileName.Length)
						varheader[5 + i] = (byte)0x00;
					else
						varheader[5 + i] = (byte)fileName[i];
				}
				varheader[15] = (byte)((fileSize + 2) & 0xff);
				varheader[16] = (byte)(((fileSize + 2)>>8) & 0xff);
				varheader[17] = (byte)(fileSize & 0xff);
				varheader[18] = (byte)((fileSize >> 8) & 0xff);

				for (int i = 0; i < fileSize; i++)
					sum += romData[i];
				for (int i = 0; i < varheader.Length; i++)
					sum += varheader[i];
				checksum[0] = (byte)((sum) & 0xff);
				checksum[1] = (byte)((sum>>8) & 0xff);

				fileSize = header.Length + varheader.Length + romData.Length + checksum.Length;

				byte[] allData = new byte[fileSize];
				Buffer.BlockCopy(header, 0, allData, 0, header.Length);
				Buffer.BlockCopy(varheader, 0, allData, header.Length, varheader.Length);
				Buffer.BlockCopy(romData, 0, allData, header.Length + varheader.Length, romData.Length);
				Buffer.BlockCopy(checksum, 0, allData, header.Length + varheader.Length + romData.Length, checksum.Length);

				using (var fs = new FileStream(fileName + ".8xv", FileMode.Create, FileAccess.Write)) {
					fs.Write(allData, 0, allData.Length);
					
				}
			}
		}
	}
}
