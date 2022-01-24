awsome_protocol = Proto("RAW_ETH", "RAW_ETH PROTOCOL") 
L = ProtoField.uint16("RAW_ETH.Length", "Length", base.DEC)
D = ProtoField.bytes("RAW_ETH.Data", "Data", base.NONE)
P = ProtoField.uint16("RAW_ETH.Pading", "Pading bytes", base.DEC)

awsome_protocol.fields = {L, D, P}

function awsome_protocol.dissector(buffer, pinfo, tree)
    length = buffer:len();
    pinfo.cols.protocol = awsome_protocol.name
    local subtree = tree:add(awsome_protocol, buffer(), "RAW_ETH PROTOCOL DATA")
    data_length = buffer(0,2):uint()
    subtree:add(L, buffer(0,2))
    subtree:add(D, buffer(2, data_length))
    subtree:add(P, length - data_length - 2)


end

ether_table = DissectorTable.get("ethertype")
ether_table:add(0x9999, awsome_protocol)