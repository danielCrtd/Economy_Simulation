#include <stdio.h>
#include <malloc.h>
#include <algorithm>
#include <math.h>

struct order
{
    float price;
    float qty;
    int id;
    order(){}
    order(float pr,float qt)
    {
        price=pr;
        qty=qt;
    }
    order(float pr,float qt,int i)
    {
        price=pr;
        qty=qt;
        id=i;
    }
    bool operator<(const order& b) const
    {
        return price<b.price;
    }
    bool operator>(const order& b) const
    {
        return price>b.price;
    }
};

class market
{
    private:
    order* trades;
    float* tradeQty;
    int cid;
    float total_bid;

    float eqid;
    float eqb;

    public:
    market(){}
    void set(int size)
    {
        trades=(order*)malloc(size*sizeof(order));
        tradeQty=(float*)malloc(size*sizeof(float));
        for(int i=0;i<size;i++)
        {
            tradeQty[i]=0.0f;
        }
        //printf("eeg\n");
        total_bid=0.0f;
        cid=0;
    }
    void ask(float price,float qty,int id)
    {
        trades[cid]=order(price,qty,id);
        cid++;
    }
    void ask(order od)
    {
        trades[cid]=od;
        cid++;
    }
    float AskQty(const order& od) const
    {
        return tradeQty[od.id];
    }
    void bid(float price,float qty,int id)
    {
        trades[cid]=order(price,qty,id);
        cid++;
        total_bid+=qty;
    }
    void bid(order od)
    {
        trades[cid]=od;
        cid++;
        total_bid+=od.qty;
    }
    float BidQty(const order& od) const
    {
        return od.qty-tradeQty[od.id];
    }
    float price()
    {
        for(int i=0;i<cid;i++)
        {
            tradeQty[i]=0;
        }
        std::sort(trades,trades+cid);
        float balance=-total_bid;
        if(balance>=0)
        {
            return 0;
        }
        for(int i=0;i<cid;i++)
        {
            balance+=trades[i].qty;
            if(balance>=0)
            {
                tradeQty[trades[i].id]=trades[i].qty-balance;
                //printf("%f %f %f %d\n",balance,trades[i].price,tradeQty[i],i);
                return trades[i].price;
            }
            tradeQty[trades[i].id]=trades[i].qty;
        }
        return (float)(1<<30);
    }
};

class RegionMarket
{
public:
    static const int numGoods=8;
    market markets[numGoods];
    float prices[numGoods];
    RegionMarket(){}
    void set(int numParts)
    {
        //printf("eee");
        for(int i=0;i<numGoods;i++)
        {
            //printf("%d\n",i);
            markets[i].set(numParts);
        }
    }

    void ask(const order& o,int good)
    {
        markets[good].ask(o);
    }
    void bid(const order& o,int good)
    {
        markets[good].bid(o);
    }
    float AskQty(const order& o,int good)
    {
        return markets[good].AskQty(o);
    }
    float BidQty(const order& o,int good)
    {
        return markets[good].BidQty(o);
    }

    void calcPrice()
    {
        for(int i=0;i<numGoods;i++)
        {
            prices[i]=markets[i].price();
        }
    }
};

class ProductionMethod
{
public:
    int numTypesI;
    int TypesI[RegionMarket::numGoods];
    float QtyI[RegionMarket::numGoods];
    int numTypesO;
    int TypesO[RegionMarket::numGoods];
    float QtyO[RegionMarket::numGoods];
    ProductionMethod(){}
    ProductionMethod(int NI,int NO)
    {
        numTypesI=NI;
        numTypesO=NO;
    }
    ProductionMethod(int NI,int NO,int* TI,int* TO,float* QI,float* QO)
    {
        numTypesI=NI;
        numTypesO=NO;
        for(int i=0;i<RegionMarket::numGoods;i++)
        {
            TypesI[i]=TI[i];
            TypesO[i]=TO[i];
            QtyI[i]=QI[i];
            QtyO[i]=QO[i];
        }
    }
    
    float Revenue(const RegionMarket& crm) const
    {
        float r=0;
        for(int i=0;i<numTypesO;i++)
        {
            r+=crm.prices[TypesO[i]]*QtyO[i];
        }
        return r;
    }
    float Costs(const RegionMarket& crm) const
    {
        float r=0;
        for(int i=0;i<numTypesI;i++)
        {
            r+=crm.prices[TypesI[i]]*QtyI[i];
        }
        return r;
    }
    float Profit(const RegionMarket& crm) const
    {
        return Revenue(crm)-Costs(crm);
    }

};

class producer
{
    static constexpr float PAFactor=1.0f;
public:
    ProductionMethod pm;
    float Cash=1000.0f;
    order orders[RegionMarket::numGoods];
    float reserves[RegionMarket::numGoods];
    float MaxReserves=40.0f;
    producer(){}
    producer(ProductionMethod p,float C)
    {
        pm=p;
        Cash=C;
        for(int i=0;i<RegionMarket::numGoods;i++)
        {
            reserves[i]=0;
        }
    }

    float reserveOrder(int good) const
    {
        return 1-2*(reserves[good]/MaxReserves);
    }
    float reserveSell(int good) const
    {
        return 2*pm.QtyO[0]*reserves[good]/MaxReserves;
    }
    
    void request(RegionMarket& crm, int id)
    {
        //float d=pm.Profit(crm);
        float r=pm.Revenue(crm)/pm.Costs(crm);
        r=sqrt(r);
        //printf("%f\n",d);
        for(int i=0;i<pm.numTypesI;i++)
        {
            int good=pm.TypesI[i];
            float NewPrice=crm.prices[good]*r;
            orders[i]=order(lerp(crm.prices[good],NewPrice,PAFactor),pm.QtyI[i]+reserveOrder(good),id);
            crm.bid(orders[i],good);
        }
        for(int i=0;i<pm.numTypesO;i++)
        {
            int good=pm.TypesO[i];
            float NewPrice=crm.prices[good]/r;
            orders[i+pm.numTypesI]=order(lerp(crm.prices[good],NewPrice,PAFactor),reserveSell(good),id);
            //printf("%f %f\n",orders[i+pm.numTypesI].qty,orders[i+pm.numTypesI].price);
            crm.ask(orders[i+pm.numTypesI],good);
        }
        
    }

    void update(RegionMarket& crm, int id)
    {
        for(int i=0;i<pm.numTypesI;i++)
        {
            int good=pm.TypesI[i];
            float QtyR=crm.BidQty(orders[i],good);
            reserves[good]+=QtyR;
            Cash-=QtyR*crm.prices[good];
        }
        for(int i=0;i<pm.numTypesO;i++)
        {
            int good=pm.TypesO[i];
            float QtyS=crm.AskQty(orders[i+pm.numTypesI],good);
            //printf("%f\n",QtyS);
            if(abs(QtyS)>10.0f)
            {
                //order k=orders[i+pm.numTypesI];
                //printf("%f %f %d\n",k.price,k.qty,k.id);
            }
            reserves[good]-=QtyS;
            Cash+=QtyS*crm.prices[good];
        }
        if(Cash<0)
        {
            //printf("%d\n",id);
        }
        if(pm.Profit(crm)>0)
        {
            for(int i=0;i<pm.numTypesI;i++)
            {
                int good=pm.TypesI[i];
                if(reserves[good]<pm.QtyI[i])
                {
                    return;
                }
            }
            for(int i=0;i<pm.numTypesI;i++)
            {
                int good=pm.TypesI[i];
                reserves[good]-=pm.QtyI[i];
            }
            for(int i=0;i<pm.numTypesO;i++)
            {
                int good=pm.TypesO[i];
                reserves[good]+=pm.QtyO[i];
                //printf("%f %f %d\n",reserves[good],pm.QtyO[good],good);
            }
        }
    }

};

int main()
{
    RegionMarket crm;
    float PMsI[8][8]={
        {1.0f},
        {1.0f,1.0f},
        {1.0f},
        {1.0f},
        {1.0f,1.0f,1.0f},
        {1.0f,1.0f}
    };
    float PMsO[8][8]={
        {1.0f},
        {1.0f},
        {1.0f},
        {1.0f},
        {1.0f},
        {5.0f}
    };
    int PMsIT[8][8]={
        {5},
        {5,0},
        {5},
        {5},
        {5,2,3},
        {1,4}
    };
    int PMsOT[8][8]={
        {0},
        {1},
        {2},
        {3},
        {4},
        {5}
    };
    int PMsITn[8]={1,2,1,1,3,2};
    int PMsOTn[8]={1,1,1,1,1,1};
    ProductionMethod PMs[6];
    producer Factories[6];

    float Inprices[6]={1.0f,2.0f,1.0f,1.0f,3.0f,0.05f};
    for(int i=0;i<6;i++)
    {
        PMs[i]=ProductionMethod(PMsITn[i],PMsOTn[i],PMsIT[i],PMsOT[i],PMsI[i],PMsO[i]);
        Factories[i]=producer(PMs[i],1000.0f);
        crm.prices[i]=Inprices[i];
        for(int j=0;j<PMs[i].numTypesO;j++)
        {
            int good=PMs[i].TypesO[j];
            Factories[i].reserves[good]=0.48f*Factories[i].MaxReserves;
        }
    }
    
    while(true)
    {
        crm.set(10);
        for(int i=0;i<6;i++)
        {
            Factories[i].request(crm,i);
        }
        crm.calcPrice();
        for(int i=0;i<6;i++)
        {
            Factories[i].update(crm,i);
        }
        for(int i=0;i<6;i++)
        {
            printf("%f ",crm.prices[i]/crm.prices[0]);
        }
        printf("\n");
        float M2=0.0f;
        for(int i=0;i<6;i++)
        {
            float r=Factories[i].Cash;
            //printf("%f ",r);
            M2+=r;
        }
        //printf("  %f\n",M2);
    }

    /*ProductionMethod pm(2,1);
    pm.TypesI[0]=0;
    pm.TypesI[1]=1;
    pm.TypesO[0]=2;
    pm.QtyI[0]=1.0f;
    pm.QtyI[1]=1.0f;
    pm.QtyO[0]=1.0f;

    producer f(pm,100.0f);
    RegionMarket crm;
    crm.prices[0]=13;
    //printf("%f\n",crm.prices[0]);
    crm.prices[1]=15;
    crm.prices[2]=30;
    //printf("%f %f %f\n",crm.prices[0],crm.prices[1],crm.prices[2]);
    //printf("eee");
    while(true)
    {
        //printf("eee");
        crm.set(5);
        //printf("efe\n");
        f.request(crm,0);
        //printf("eff\n");
        crm.ask(order(10,10,1),0);
        crm.ask(order(10,10,2),1);
        crm.bid(order(30,10,3),2);
        //printf("efg\n");
        crm.calcPrice();
        f.update(crm,0);
        printf("%f %f %f %f\n",crm.prices[0],crm.prices[1],crm.prices[2],f.Cash);
    }*/

    return 0;
}