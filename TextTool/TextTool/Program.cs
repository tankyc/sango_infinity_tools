using System;
using System.Collections.Generic;
using System.Linq;
//using Microsoft.Office.Core;
using System.IO;
using System.Xml;
using System.Text;
using Newtonsoft.Json;
using Newtonsoft.Json.Linq;

namespace TextTool
{
    class Program
    {

        static void Main(string[] args)
        {
            DateTime beforDT = System.DateTime.Now;
            var arguments = CommandLineArgumentParser.Parse(args);
            string outPath = "";
            if (arguments.Has("-out"))
            {
                outPath = arguments.Get("-out").Next;
                Console.WriteLine("out dir：{0}", outPath);
            }

            string src = "";
            if (arguments.Has("-src"))
            {
                src = arguments.Get("-src").Next;
                Console.WriteLine("src dir：{0}", src);
            }

            string combine_file = "";
            if (arguments.Has("-combine"))
            {
                combine_file = arguments.Get("-combine").Next;
                Console.WriteLine("combine_file：{0}", src);
            }

            JObject root = null;
            if (!string.IsNullOrEmpty(combine_file))
                root = new JObject();

            JsonExcelMerge textMerge = new JsonExcelMerge();
            if (!Path.IsPathRooted(src))
            {
                src = Path.GetFullPath(src);
            }
            if (!Path.IsPathRooted(outPath))
            {
                outPath = Path.GetFullPath(outPath);
            }

            if (File.Exists(src))
            {
                textMerge.ExportFile(src, outPath);
            }
            else
            {
                textMerge.ExportDir(src, outPath, root);
            }

            if (!string.IsNullOrEmpty(combine_file))
            {
                File.WriteAllText($"{outPath}/{combine_file}.json", root.ToString());
            }

            Console.WriteLine("处理完成 !!");

            DateTime afterDT = System.DateTime.Now;
            TimeSpan ts = afterDT.Subtract(beforDT);
            Console.WriteLine("总共花费{0}ms.", ts.TotalMilliseconds);



            //Console.ReadKey();
        }



        public class CommandLineArgument
        {
            List<CommandLineArgument> _arguments;

            int _index;

            string _argumentText;

            public CommandLineArgument Next
            {
                get
                {
                    if (_index < _arguments.Count - 1)
                    {
                        return _arguments[_index + 1];
                    }

                    return null;
                }
            }
            public CommandLineArgument Previous
            {
                get
                {
                    if (_index > 0)
                    {
                        return _arguments[_index - 1];
                    }

                    return null;
                }
            }
            internal CommandLineArgument(List<CommandLineArgument> args, int index, string argument)
            {
                _arguments = args;
                _index = index;
                _argumentText = argument;
            }

            public CommandLineArgument Take()
            {
                return Next;
            }

            public IEnumerable<CommandLineArgument> Take(int count)
            {
                var list = new List<CommandLineArgument>();
                var parent = this;
                for (int i = 0; i < count; i++)
                {
                    var next = parent.Next;
                    if (next == null)
                        break;

                    list.Add(next);

                    parent = next;
                }

                return list;
            }

            public static implicit operator string(CommandLineArgument argument)
            {
                return argument._argumentText;
            }

            public override string ToString()
            {
                return _argumentText;
            }
        }

        public class CommandLineArgumentParser
        {

            List<CommandLineArgument> _arguments;
            public static CommandLineArgumentParser Parse(string[] args)
            {
                return new CommandLineArgumentParser(args);
            }

            public CommandLineArgumentParser(string[] args)
            {
                _arguments = new List<CommandLineArgument>();

                for (int i = 0; i < args.Length; i++)
                {
                    _arguments.Add(new CommandLineArgument(_arguments, i, args[i]));
                }

            }

            public CommandLineArgument Get(string argumentName)
            {
                return _arguments.FirstOrDefault(p => p == argumentName);
            }

            public bool Has(string argumentName)
            {
                return _arguments.Count(p => p == argumentName) > 0;
            }
        }
    }
}
