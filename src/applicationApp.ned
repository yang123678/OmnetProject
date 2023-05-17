//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package baseline_aoi;;

import inet.applications.contract.IApp;


simple ApplicationApp like IApp
{
    parameters:
        string interfaceTableModule;
        string multicastInterface = default("wlan");   // The path to the InterfaceTable module

        double period @unit(s) = default(0.1s);
       
        int localPort = default(1008);
        
        int PacketSize = default(50);
//        string destAddress;
        
    gates:
        input socketIn @labels(UdpControlInfo/up);
        output socketOut @labels(UdpControlInfo/down);
}
