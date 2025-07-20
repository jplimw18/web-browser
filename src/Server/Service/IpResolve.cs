using System.Runtime.InteropServices;

namespace Server.Service;

public class IpResolve
{
    [DllImport("../Core/Build/ip_resolve.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int DnsTest(ushort dns_t);
}
