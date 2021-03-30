# Special readout with hadaq::MonitorModule

It is DABC component, which can readout values with "trbcmd"
Such data packed into fully separate event with single subevent.
Specially configured TrbProcessor can extract data and put into
event structure. second.C shows how such event structure can
be read and processed.

In parallel one can do normal TDC analysis.
