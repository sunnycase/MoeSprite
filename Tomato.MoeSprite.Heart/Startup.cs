using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace Tomato.MoeSprite.Heart
{
    public static class Startup
    {
        public static void CreateApplication([MarshalAs(UnmanagedType.IUnknown)] out object obj)
        {
            Console.WriteLine("Hello World.");
            obj = new object();
        }
    }
}
