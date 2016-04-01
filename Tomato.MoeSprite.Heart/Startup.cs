using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace Tomato.MoeSprite.Heart
{
    public static class Startup
    {
        public static void CreateApplication(out IApplication app)
        {
            app = new Application();
        }
    }
    
    public interface IApplication
    {
        void Run();
    }

    public class Application : IApplication
    {
        public Application()
        {

        }

        public void Run()
        {
            Console.WriteLine("Hello World.");
        }
    }
}
