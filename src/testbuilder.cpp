#include "testbuilder.h"
#include "pin.h"
#include <sstream>
#include <iostream>
#include <fstream>

using namespace Novorado::Partition;

Novorado::Partition::TestBuilder::TestBuilder(const std::string &fn)
{
    H = std::make_shared<KLFM>();

    ReadGraphFromFile(fn);
}

Pin &Novorado::Partition::TestBuilder::getNextPin(Cell &c1, const std::string &name)
{
    c1.m_Pins.emplace_back();
    Pin& newPin=c1.m_Pins.back();
    newPin.SetId(c1.m_Pins.size());
    newPin.SetCell(&c1);
    newPin.SetName(name); // that will trigger consistency check
    return newPin;
}

void Novorado::Partition::TestBuilder::MakeCell(const std::string &cellName, const std::string &ssq, unsigned int &idx)
{
#ifdef CHECK_LOGIC
    if(m_name2cell.find(cellName)!=m_name2cell.end()) {
        std::stringstream msg;
        msg << "Duplicated cell '" << cellName << "' at line " << current_ln;
        throw std::logic_error(msg.str());
    }
#endif
    Square sq=0;
    std::stringstream s(ssq);
    s >> sq;
    Cell tmpCell;
    tmpCell.SetId(idx++);
    tmpCell.SetName(cellName);
    tmpCell.SetSquare(sq);

    H->m_AllCells->push_back(tmpCell);
    H->instances.init(&(*H->m_AllCells)[0],H->m_AllCells->size());

    H->m_AllCells->back().SetPartition(&H->p0);
#ifdef CHECK_LOGIC
    if(!sq) {
        std::stringstream msg;
        msg << "Cell '" << cellName << "' has illegal square '" << ssq << "'";
        throw std::logic_error(msg.str());
    }
#endif
    // Quadratic complexity readressing, bad.
    for(auto& cell: *H->m_AllCells)
    {
        m_name2cell[cell.GetName()]=&cell;
    }
}

void Novorado::Partition::TestBuilder::MakeNet(Net &net, const std::vector<std::string> &words, int idx)
{
    net.SetId(idx);
    int cnt=0;
    Weight w;
    std::stringstream s;
    std::string str1;
    for(std::string str:words)
    {
        switch(cnt++)
        {
        case 0:
            net.SetName(str);
#ifdef  ALGORITHM_VERBOSE
            std::cout << "Creating net '" << str << "' Weight="  << std::flush;
#endif
            break;
        case 1: s.str(str);
            s >> w;
#ifdef  ALGORITHM_VERBOSE
            std::cout << w << " " << std::flush;
#endif
            net.SetWeight(w);
            break;
        default:
            if(!str1.length()) str1=str;
            else MakePin(str1,str,net),str1.clear();
            break;
        }
    }
#ifdef  ALGORITHM_VERBOSE
    std::cout << std::endl;
#endif
}

void Novorado::Partition::TestBuilder::MakePin(const std::string &cellName, const std::string &pinName, Net &net)
{
    auto i=m_name2cell.find(cellName);
#ifdef CHECK_LOGIC
    if(i==m_name2cell.end()) {
        throw std::logic_error("Incorrect cell name");
    }
#endif
    Pin& pin=getNextPin(*i->second,pinName);
    pin.SetNet(&net);
    net.AddPin(&pin);
#ifdef  ALGORITHM_VERBOSE
    std::cout << cell.GetName() << ":" << pin.GetName() << " ";
#endif
}

void Novorado::Partition::TestBuilder::ReadGraphFromFile(const std::string &fn)
{
    std::cout << "Reading from '" << fn << "' .. " << std::flush;
    std::ifstream f(fn.c_str());
    current_ln=0;
    unsigned int cell_idx=0;
    std::vector< std::vector<std::string> > tmpNets;
    while(!f.eof())
    {
        current_ln++;
        std::string l;
        std::vector<std::string> words;
        std::getline(f,l);
        if(f.eof() && l=="") break;
        std::stringstream ss(l);
        while (ss>>l) words.push_back(l);
#ifdef CHECK_LOGIC
        if(words.size()<2) {
            std::cout << "Error at " << fn << ":" << current_ln
                      << " '" << l << "'" << std::endl;
            throw std::logic_error("Parsing error");
        }
#endif
        if(words.size()==2)
        {
#ifdef CHECK_LOGIC
            if(
                    words.front().substr(0,5) == "fixed" &&
                    m_name2cell.find(words.back()) == m_name2cell.end()
                    ){
                throw std::logic_error("Invalid input file");
            }
#endif//CHECK_LOGIC
            if(words.front()=="fixedleft")
            {
                Cell* c=(m_name2cell[words.back()]);
                c->SetFixed();
                c->SetPartition(&H->p0);
                continue;
            } else if(words.front()=="fixedright")
            {
                Cell* c=(m_name2cell[words.back()]);
                c->SetFixed();
                c->SetPartition(&H->p1);
                continue;
            }

            MakeCell(words[0],words[1],cell_idx);
        } else tmpNets.push_back(words);
    }
    f.close();

    unsigned int netIdx=0;
    H->nets.resize(tmpNets.size());

    for(std::vector< std::vector<std::string> >::iterator k=tmpNets.begin();
        k!=tmpNets.end();k++,netIdx++)
        MakeNet(H->nets[netIdx],*k,netIdx);

    std::cout << " done" << std::endl;
}
