#include <iostream>
#include "llvm/IR/ConstantRange.h"

std::vector<llvm::APInt> concretize_function(const llvm::ConstantRange& range)
{
    if(range.isEmptySet())
    {
        return std::vector<llvm::APInt>{};
    }

    if(range.isFullSet())
    {
        auto w = range.getBitWidth();
        auto Min = llvm::APInt::getSignedMinValue(w);
        auto Max = llvm::APInt::getSignedMaxValue(w);
        
        std::vector<llvm::APInt> res;
        for(llvm::APInt val = Min; val.slt(Max); val = val + 1) {
            res.push_back(val);
        }
        res.push_back(Max);
        return res;
    }

    std::vector<llvm::APInt> res;
    for(llvm::APInt val = range.getLower(); val.slt(range.getUpper()); val = val + 1) {
        res.push_back(val);
    }
    return res;
}

llvm::ConstantRange abstract_function(const std::vector<llvm::APInt> & set,uint32_t bitwidth)
{
    if(set.empty())
    {
        // empty set
        return llvm::ConstantRange(bitwidth,false);
    }

    auto Min = llvm::APInt::getSignedMaxValue(bitwidth);
    auto Max = llvm::APInt::getSignedMinValue(bitwidth);

    for(const auto & val : set)
    {
        if(val.sle(Min))
        {
            Min = val;
        }

        if(Max.sle(val))
        {
            Max = val;
        }
    }

    if(Min.eq(llvm::APInt::getSignedMinValue(bitwidth)) && Max.eq(llvm::APInt::getSignedMaxValue(bitwidth)))
    {
        // full set
        return llvm::ConstantRange(bitwidth,true);
    }else{
         return llvm::ConstantRange(Min,Max + 1);
    }
}

void print_range(const llvm::ConstantRange& range)
{
    llvm::outs() << "[";
    range.getLower().print(llvm::outs(),true);
    llvm::outs() << ",";
    range.getUpper().print(llvm::outs(),true);
    llvm::outs() << ")";
}

llvm::ConstantRange naive_srem(const llvm::ConstantRange& LHS, const llvm::ConstantRange& RHS)
{
    auto s = LHS.sdiv(RHS); // s = lhs / rhs
    //print_range(s);
    s = s.multiply(RHS); // s = s * rhs
    //print_range(s);
    s = LHS.sub(s); // s = lhs - s
    //print_range(s);
    return s;
}

bool as_precise_as(const llvm::ConstantRange& LHS, const llvm::ConstantRange& RHS)
{
    if(RHS.isFullSet() || LHS.isEmptySet())
        return true;
    return RHS.getLower().sle(LHS.getLower()) && LHS.getUpper().sle(RHS.getUpper());
}

bool is_more_precise(const llvm::ConstantRange& LHS, const llvm::ConstantRange& RHS)
{
    if(RHS.isFullSet() && !LHS.isFullSet())
        return true;
    if(LHS.isEmptySet() && !RHS.isEmptySet())
        return true;
    
    bool res = RHS.getLower().sle(LHS.getLower()) && LHS.getUpper().sle(RHS.getUpper());

    res &= !(RHS.getLower().eq(LHS.getLower()) && LHS.getUpper().eq(RHS.getUpper()));

    return res;
}

void exclusively_test(uint32_t bitwidth)
{
    // generate all possible ranges for bitwidth n
    auto Min = llvm::APInt::getSignedMinValue(bitwidth);
    auto Max = llvm::APInt::getSignedMaxValue(bitwidth);

    std::vector<llvm::ConstantRange> abstract_values;
    abstract_values.push_back(llvm::ConstantRange(bitwidth,false));
    //print_range(llvm::ConstantRange(bitwidth,false));

    for(auto lower = Min; lower.slt(Max); lower = lower + 1)
    {
        for(auto upper = lower + 1; upper.slt(Max); upper = upper + 1)
        {
            auto range = llvm::ConstantRange(lower,upper);
            //print_range(range);
            abstract_values.push_back(range);
        }
        auto range = llvm::ConstantRange(lower,Max);
        //print_range(range);
        abstract_values.push_back(range);
    }
    print_range(llvm::ConstantRange(bitwidth,true));
    llvm::outs() << "\n";
    abstract_values.push_back(llvm::ConstantRange(bitwidth,true));


    auto count1 = 0;
    auto count2 = 0;
    auto count3 = 0;

    for(uint32_t i = 0; i < abstract_values.size(); i ++)
    {
        for(uint32_t j = 0; j < abstract_values.size(); j ++)
        {
            print_range(abstract_values[i]);
            llvm::outs() << ",";
            print_range(abstract_values[j]);
            llvm::outs() << ":\n";

            auto res1 = abstract_values[i].srem(abstract_values[j]);
            print_range(res1);

            auto res2 = naive_srem(abstract_values[i],abstract_values[j]);
            print_range(res2);

            auto concre_set1 = concretize_function(abstract_values[i]);
            auto concre_set2 = concretize_function(abstract_values[j]);
            std::vector<llvm::APInt> concre_set3;

            [&](){
                for(auto & concre1 : concre_set1)
                {
                    for(auto & concre2 : concre_set2)
                    {
                        // reminder = 0 
                        if(concre2 == 0)
                        {
                            //return empty set
                            concre_set3.clear();
                            return;
                        }
                        auto concre3 = concre1.srem(concre2);
                        auto concre4 = concre1.sdiv(concre2);
                        concre4 = concre4 * concre2;
                        concre4 = concre1 - concre4;
                        if(!concre4.eq(concre3))
                        {
                            assert(false);
                            concre3.print(llvm::outs(),true);
                            llvm::outs() << " , ";
                            concre4.print(llvm::outs(),true);
                            assert(false);
                        }else{

                        }
                        concre_set3.push_back(concre3);
                    }
                }
            }();

            auto res3 = abstract_function(concre_set3,bitwidth);
            print_range(res3);

            assert(as_precise_as(res3,res1));

            bool b1 = is_more_precise(res2,res1);
            bool b2 = is_more_precise(res1,res2);

            if(b1)
            {
                count1 ++ ;
            }

            if(b2)
            {
                count2 ++;
            }

            if(!b1&&!b2)
            {
                count3 ++;
            }
            // if(is_more_precise(res3,res1))
            // {
            //     std::cerr << "++++++++++++++++++++++++++++++++++++";
            // }
            llvm::outs() << "\n";
        }
    }

    llvm::outs() << count1 << "\n";
    llvm::outs() << count2 << "\n";
    llvm::outs() << count3 << "\n";
}

int main()
{
    //auto lhs = llvm::ConstantRange(llvm::APInt(32,-16,true));
    //auto rhs = llvm::ConstantRange(llvm::APInt(32,-1,true));
    //print_range(lhs.srem(rhs));
    //auto res = naive_srem(lhs,rhs);
    exclusively_test(4);
}