/*
 * Copyright (c) 2008 Hrvoje Zeba <zeba.hrvoje@gmail.com>
 *
 *    This file is part of Antiaim.
 *
 *    Antiaim is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Antiaim is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Antiaim; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#ifndef ANN_H
#define ANN_H

#include <string>
#include <vector>

class ann {
public:
    typedef unsigned int uint;
        
    typedef std::vector<uint>               uint_vec;
    typedef std::vector<double>             double_vec;
        
    typedef std::pair<double_vec, double_vec> train_data;
    typedef std::vector<train_data>         train_set;

    ann();
    virtual ~ann();

    bool initialize(const uint_vec& layout);

    bool calculate(const double_vec& input);
    bool get_output(double_vec& output);

    bool train(const train_set& data);
        
    bool save(const std::string& file);
    bool load(const std::string& file);

protected:
    void calculate_errors(const double_vec& ideal_output);
    void update_weights();

    virtual double o_error(double t_k, double o_k);
    virtual double u_error(double o_h, double sum);
        
    virtual double e_error(double_vec tk, double_vec ok);

    virtual double delta_w(double error, double x, double prev_w);
        
    virtual double sigma(double net);

protected:
    typedef std::pair<uint, uint>     layer_offset;
    typedef std::vector<layer_offset> layer_offsets;
        
    uint_vec  m_layer_layout;
    uint      m_bias;

    layer_offsets m_layer_offsets;
        
    double_vec m_delta_w;
    double_vec m_weights;
        
    double_vec m_outputs;
    double_vec m_errors;
};

#endif // ANN_H
