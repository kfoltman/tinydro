#pragma once

class Encoder
{
    int id;
    int last;
public:
    Encoder(int _id);
    void init();
    int get();
    int delta();
};