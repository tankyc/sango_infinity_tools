using Excel;
using System;
using System.Collections.Generic;
using System.Data;
using System.IO;
using System.Text;
using System.Xml;

namespace TextTool
{
    class TextExcelMerge
    {
        public enum ValueType : int
        {
            none = 0,
            boolean,
            int8,
            uint8,
            int16,
            uint16,
            int32,
            uint32,
            int64,
            uint64,
            float_,
            double_,
            string_array,
            bool_array,
            int8_array,
            uint8_array,
            int16_array,
            uint16_array,
            int32_array,
            uint32_array,
            int64_array,
            uint64_array,
            float_array,
            double_array,
        }
        struct PropertyData
        {
            public string name;
            public ValueType pType;
            public int cell;
        }

        public class ExportMethod
        {
            public static ValueType str2VType(string type)
            {
                switch (type)
                {
                    case "s":
                        return ValueType.none;
                    case "b":
                        return ValueType.boolean;
                    case "i8":
                        return ValueType.int8;
                    case "u8":
                        return ValueType.uint8;
                    case "i16":
                        return ValueType.int16;
                    case "u16":
                        return ValueType.uint16;
                    case "i32":
                        return ValueType.int32;
                    case "u32":
                        return ValueType.uint32;
                    case "i64":
                        return ValueType.int64;
                    case "u64":
                        return ValueType.uint64;
                    case "f":
                        return ValueType.float_;
                    case "d":
                        return ValueType.double_;
                    case "as":
                        return ValueType.string_array;
                    case "ab":
                        return ValueType.int8_array;
                    case "ai8":
                        return ValueType.int8_array;
                    case "au8":
                        return ValueType.uint8_array;
                    case "ai16":
                        return ValueType.int16_array;
                    case "au16":
                        return ValueType.uint16_array;
                    case "ai32":
                        return ValueType.int32_array;
                    case "au32":
                        return ValueType.uint32_array;
                    case "ai64":
                        return ValueType.int64_array;
                    case "au64":
                        return ValueType.uint64_array;
                    case "af":
                        return ValueType.float_array;
                    case "ad":
                        return ValueType.double_array;
                    default:
                        return ValueType.none;
                }
            }

            //public ValueType type;
            //public string value;
            public static void Write(BinaryWriter bw, ValueType type, string value)
            {
                char[] spechar = new[] { '&' };
                switch (type)
                {
                    case ValueType.none:
                        {
                            bw.Write(value);
                            break;
                        }
                    case ValueType.boolean:
                        {
                            bool v = false;
                            if (value == "0")
                                v = false;
                            else if (value == "1")
                                v = true;
                            else
                                bool.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.int8:
                        {
                            sbyte v = 0;
                            sbyte.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.uint8:
                        {
                            byte v = 0;
                            byte.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.int16:
                        {
                            short v = 0;
                            short.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.uint16:
                        {
                            ushort v = 0;
                            ushort.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.int32:
                        {
                            int v = 0;
                            int.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.uint32:
                        {
                            uint v = 0;
                            uint.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.int64:
                        {
                            long v = 0;
                            long.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.uint64:
                        {
                            ulong v = 0;
                            ulong.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.float_:
                        {
                            float v = 0;
                            float.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.double_:
                        {
                            double v = 0;
                            double.TryParse(value, out v);
                            bw.Write(v);
                            break;
                        }
                    case ValueType.string_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.none, array[i]);
                            }
                            break;
                        }
                    case ValueType.bool_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.boolean, array[i]);
                            }
                            break;
                        }
                    case ValueType.int8_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.int8, array[i]);
                            }
                            break;
                        }
                    case ValueType.uint8_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.uint8, array[i]);

                            }
                            break;
                        }
                    case ValueType.int16_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.int16, array[i]);

                            }
                            break;
                        }
                    case ValueType.uint16_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.uint16, array[i]);

                            }
                            break;
                        }
                    case ValueType.int32_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.int32, array[i]);

                            }
                            break;
                        }
                    case ValueType.uint32_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.uint32, array[i]);

                            }
                            break;
                        }
                    case ValueType.int64_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.int64, array[i]);

                            }
                            break;
                        }
                    case ValueType.uint64_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.uint64, array[i]);

                            }
                            break;
                        }
                    case ValueType.float_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.float_, array[i]);

                            }
                            break;
                        }
                    case ValueType.double_array:
                        {
                            string[] array = value.Split(spechar);
                            bw.Write((ushort)array.Length);
                            for (int i = 0; i < array.Length; i++)
                            {
                                Write(bw, ValueType.double_, array[i]);

                            }
                            break;
                        }
                }
            }

        }

        //public void Export(Microsoft.Office.Interop.Excel.Worksheet sheet, string savePath = "")
        public void Export(DataTable sheet, string savePath = "")
        {
            if (sheet != null)
            {
                Console.WriteLine("正在处理 Sheet：" + sheet.TableName);
                List<PropertyData> propertyData = new List<PropertyData>();
                string data_name = sheet.TableName;
                int check_cell = 3;
                int export_cell_index = 0;
                DataRow row_4 = sheet.Rows[3];
                DataRow row_3 = sheet.Rows[2];
                string content = row_4[export_cell_index].ToString();
                while (!string.IsNullOrEmpty(content))
                {
                    content = content.Trim();
                    if (content.Length > 0 && content != "#")
                    {
                        PropertyData d = new PropertyData()
                        {
                            name = row_3[export_cell_index].ToString(),
                            cell = export_cell_index,
                            pType = ExportMethod.str2VType(content)
                        };
                        propertyData.Add(d);
                        if (propertyData.Count == 1)
                            check_cell = d.cell;

                    }
                    export_cell_index++;
                    try
                    {
                        content = row_4[export_cell_index].ToString();
                    }
                    catch (Exception ex)
                    {
                        break;
                    }
                }

                if (!System.IO.Directory.Exists(savePath))
                    System.IO.Directory.CreateDirectory(savePath);

                using (Stream stream = File.Open($"{savePath}/{sheet}.bin", FileMode.Create, FileAccess.Write))
                {
                    using (BinaryWriter bw = new BinaryWriter(stream))
                    {
                        // 统计数据量
                        int export_row_index = 4;
                        DataColumn col_check = sheet.Columns[check_cell];
                        int count = 0;
                        bw.Write(count);
                        bw.Write(count);
                        for (int j = 0; j < 65535; j++)
                        {
                            export_row_index = 4;
                            try
                            {
                                DataRow row_next = sheet.Rows[export_row_index + j];
                                if (row_next[check_cell].ToString().Length <= 0)
                                    break;

                                for (int i = 0; i < propertyData.Count; ++i)
                                {
                                    PropertyData p = propertyData[i];
                                    string value = row_next[p.cell].ToString();
                                    ExportMethod.Write(bw, p.pType, value);
                                    export_row_index++;
                                }
                                count++;
                            }
                            catch (Exception ex)
                            {
                                break;
                            }
                        }
                        bw.Seek(0, SeekOrigin.Begin);
                        Console.WriteLine($"{sheet.TableName} 处理了数据: {count} 个");
                        bw.Write(count);
                        bw.Write(count);
                        bw.Flush();
                    }
                    stream.Close();
                }

                using (Stream stream = File.Open($"{savePath}/{sheet.TableName}_key.bin", FileMode.Create, FileAccess.Write))
                {
                    using (BinaryWriter bw = new BinaryWriter(stream))
                    {
                        bw.Write(propertyData.Count);
                        for (int i = 0; i < propertyData.Count; ++i)
                        {
                            PropertyData p = propertyData[i];
                            bw.Write(p.name);
                            bw.Write((int)p.pType);
                        }
                        bw.Flush();
                    }
                    stream.Close();
                }
            }
        }

        public void Export(string path, bool isDir = false, string savePath = "", string combine_sheet = "")
        {
            if (isDir)
            {
                List<String> list = GetAllFile(path, "*.xlsx");
                foreach (String filename in list)
                {
                    Export(filename, false, savePath);
                }
                list = GetAllFile(path, "*.xlsm");
                foreach (String filename in list)
                {
                    Export(filename, false, savePath);
                }
                return;
            }

            if (path.StartsWith("~"))
                return;

            Console.WriteLine("正在处理：" + path);

            FileStream fileStream = new FileStream(path, FileMode.Open, FileAccess.Read);
            IExcelDataReader reader = ExcelReaderFactory.CreateOpenXmlReader(fileStream);
            DataSet result = reader.AsDataSet();

            XmlDocument xmlDocument = new XmlDocument();
            XmlNode rootNode = xmlDocument.AppendChild(xmlDocument.CreateElement("Root"));

            //ExcelEdit excel = new ExcelEdit();
            //excel.Open(path);
            if (!string.IsNullOrEmpty(combine_sheet))
            {
                string[] sheetName = combine_sheet.Split(',');
                for (int i = 0; i < sheetName.Length; i++)
                {
                    //try
                    //{
                    DataTable sheet = result.Tables[sheetName[i]];

                    //Microsoft.Office.Interop.Excel.Worksheet sheet = excel.GetSheet(sheetName[i]);
                    if (sheet != null)
                        Export(sheet, savePath);
                    //}
                    //catch (Exception ex)
                    //{

                    //}
                }
            }
            else
            {
                for (int i = 0; i < 100; i++)
                {
                    DataTable sheet = null;
                    try
                    {
                        sheet = result.Tables[i];
                    }
                    catch (Exception ex)
                    {
                        break;
                    }
                    //Microsoft.Office.Interop.Excel.Worksheet sheet = excel.GetSheet(i);
                    if (sheet == null) break;
                    Export(sheet, savePath);

                }
            }

            reader.Close();
            fileStream.Close();
            reader.Dispose();
            fileStream.Dispose();
            //excel.Close();

            if (!string.IsNullOrEmpty(combine_sheet))
            {
                string[] sheetName = combine_sheet.Split(',');
                using (Stream stream = File.Open($"{savePath}/{Path.GetFileNameWithoutExtension(path)}.bin", FileMode.Create, FileAccess.Write))
                {
                    using (BinaryWriter bw = new BinaryWriter(stream))
                    {
                        for (int i = 0; i < sheetName.Length; ++i)
                        {
                            string destFile = $"{savePath}/{sheetName[i]}.bin";
                            if (File.Exists(destFile))
                            {
                                FileInfo fileInfo = new FileInfo(destFile);
                                bw.Write(fileInfo.Length);
                                bw.Write(File.ReadAllBytes(destFile));
                            }
                        }
                        bw.Flush();
                    }
                    stream.Close();
                }
            }
        }

        public List<string> GetAllFile(string sSourcePath, string fileType = "*.*")
        {

            List<String> list = new List<string>();

            //遍历文件夹

            DirectoryInfo theFolder = new DirectoryInfo(sSourcePath);

            FileInfo[] thefileInfo = theFolder.GetFiles(fileType, SearchOption.AllDirectories);

            foreach (FileInfo NextFile in thefileInfo)  //遍历文件

                list.Add(NextFile.FullName);


            //遍历子文件夹

            DirectoryInfo[] dirInfo = theFolder.GetDirectories();

            foreach (DirectoryInfo NextFolder in dirInfo)
            {

                //list.Add(NextFolder.ToString());

                FileInfo[] fileInfo = NextFolder.GetFiles(fileType, SearchOption.AllDirectories);

                foreach (FileInfo NextFile in fileInfo)  //遍历文件

                    list.Add(NextFile.FullName);
            }

            return list;
        }
    }
}
