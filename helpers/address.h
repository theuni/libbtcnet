CAddress ConnectionToAddress(const CConnection& conn)
{
    CAddress addr;

    if(conn.IsDns())
    {
    }
    else
    {
        sockaddr_storage storage;
        int sock_size = sizeof(storage);
        conn.GetSockAddr((sockaddr*)&storage, &sock_size);
        addr.ip[10] = addr.ip[11] = 0xff;
        if(storage.ss_family == AF_INET)
        {
            sockaddr_in* addr4 = (sockaddr_in*)&storage;
            memcpy(addr.ip + 12, &addr4->sin_addr, 4);
            addr.port = ntohs(addr4->sin_port);
        }
        else if(storage.ss_family == AF_INET6)
        {
            sockaddr_in6* addr6 = (sockaddr_in6*)&storage;
            memcpy(addr.ip, &addr6->sin6_addr, 16);
            addr.port = ntohs(addr6->sin6_port);
        }
    }
    return addr;
}
