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

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <fstream>

#include "ann.h"

using namespace std;


ann::ann()
  : m_layer_layout(),
    m_bias(1),
    m_layer_offsets(),
    m_delta_w(),
    m_weights(),
    m_outputs()
{
}

ann::~ann()
{
}

bool ann::initialize(const uint_vec& layer_layout)
{
    if (layer_layout.size() < 2)
        return false;
        
    m_layer_layout = layer_layout;
    m_layer_offsets.resize(m_layer_layout.size());
        
    m_delta_w.clear();
    m_weights.clear();

    m_outputs.clear();
    m_errors.clear();

    m_layer_offsets[0] = layer_offset(-1, 0);

    int w_size = 0;
    int o_size = m_layer_layout[0] + m_bias;

    for (size_t i = 1; i < layer_layout.size(); ++i) {
        m_layer_offsets[i] = layer_offset(w_size, o_size);

        w_size += (layer_layout[i - 1] + m_bias) * layer_layout[i];
        o_size += layer_layout[i] + m_bias;
    }

    m_delta_w.resize(w_size, 0);
    m_weights.resize(w_size, 0);

    m_outputs.resize(o_size, 1);
    m_errors.resize(o_size, 0);

    srand((unsigned int)time(NULL));

    for (size_t i = 0; i < m_weights.size(); i++) {
        m_weights[i] = 1 - 2 * (double)rand() / RAND_MAX;
        m_delta_w[i] = 0;
    }

    return true;
}

bool ann::calculate(const double_vec& input)
{
    if (input.size() != m_layer_layout[0])
        return false;

    for (size_t i = 0; i < input.size(); i++)
        m_outputs[i] = sigma(input[i]);

    for (size_t curr_layer = 1; curr_layer < m_layer_layout.size(); curr_layer++) {
        layer_offset& p_off = m_layer_offsets[curr_layer - 1];
        layer_offset& c_off = m_layer_offsets[curr_layer];

        uint t_weighc_off = c_off.first;
        uint t_outpuc_off = c_off.second;

        for (uint t_node = 0; t_node < m_layer_layout[curr_layer]; t_node++) {
            uint p_outpuc_off = p_off.second;
                        
            double sum = 0;

            uint node_num = m_layer_layout[curr_layer - 1] + m_bias;

            for (uint p_node = 0; p_node < node_num; p_node++) {
                sum += m_outputs[p_outpuc_off] * m_weights[t_weighc_off];

                p_outpuc_off++;
                t_weighc_off++;
            }

            m_outputs[t_outpuc_off] = sigma(sum);

            t_outpuc_off++;
        }
    }

    return true;
}

bool ann::get_output(double_vec& output)
{
    uint output_layer = m_layer_layout.size() - 1;

    if (output.size() != m_layer_layout[output_layer])
        return false;
        
    layer_offset& last_layer = m_layer_offsets[output_layer];
    uint outpuc_off = last_layer.second;

    for (size_t i = 0; i < output.size(); i++) {
        output[i] = m_outputs[outpuc_off];
        outpuc_off++;
    }

    return true;
}

void ann::calculate_errors(const double_vec& ideal_output)
{
    uint output_layer = m_layer_layout.size() - 1;
        
    layer_offset& o_off = m_layer_offsets[output_layer];
    uint outpuc_off = o_off.second;

    for (uint o_node = 0; o_node < m_layer_layout[output_layer]; o_node++) {
        m_errors[outpuc_off] = o_error(ideal_output[o_node], m_outputs[outpuc_off]);
        outpuc_off++;
    }

    for (uint curr_layer = output_layer - 1; curr_layer > 0; curr_layer--) {
        layer_offset& c_off = m_layer_offsets[curr_layer];
        layer_offset& n_off = m_layer_offsets[curr_layer + 1];

        uint co_off = c_off.second;

        uint node_num = m_layer_layout[curr_layer] + m_bias;
                
        for (uint t_node = 0; t_node < node_num; t_node++) {
            uint nw_off = n_off.first + t_node;
            uint no_off = n_off.second;

            double sum = 0;
                
            for (uint n_node = 0; n_node < m_layer_layout[curr_layer + 1]; n_node++) {
                sum += m_weights[nw_off] * m_errors[no_off];

                nw_off += node_num;
                no_off++;
            }

            m_errors[co_off] = u_error(m_outputs[co_off], sum);
                        
            co_off++;
        }
    }
}

void ann::update_weights()
{
    for (uint curr_layer = 1; curr_layer < m_layer_layout.size(); curr_layer++) {
        layer_offset& c_off = m_layer_offsets[curr_layer];
        layer_offset& p_off = m_layer_offsets[curr_layer - 1];
                
        uint node_num = m_layer_layout[curr_layer];
                
        uint cw_off = c_off.first;
        uint co_off = c_off.second;
                
        for (uint c_node = 0; c_node < node_num; c_node++) {
            uint po_off = p_off.second;
                        
            for (uint p_node = 0; p_node < m_layer_layout[curr_layer - 1] + m_bias; p_node++) {
                m_delta_w[cw_off] = delta_w(m_errors[co_off], m_outputs[po_off], m_delta_w[cw_off]);
                m_weights[cw_off] += m_delta_w[cw_off];

                cw_off++;
                po_off++;                               
            }

            co_off++;
        }
    }
}

bool ann::train(const train_set& data)
{
    for (size_t i = 0; i < data.size(); i++) {
        if (calculate(data[i].first) == false)
            return false;

        calculate_errors(data[i].second);
        update_weights();
    }
        
    return true;
}

bool ann::save(const string& file)
{
    fstream f(file.c_str(), fstream::out);

    if (f.fail())
        return false;

    f << m_bias << ' ' << m_layer_layout.size() << ' ';

    for (size_t i = 0; i < m_layer_layout.size(); i++)
        f << m_layer_layout[i] << ' ';
        
    for (size_t i = 0; i < m_weights.size(); i++)
        f << m_weights[i] << ' ';

    f << endl;

    f.close();

    return true;
}

bool ann::load(const string& file)
{
    fstream f(file.c_str(), fstream::in);

    int layer_count, weight_count, output_count;

    if (f.fail())
        return false;

    f >> m_bias >> layer_count;

    m_layer_layout.clear();
    m_layer_offsets.resize(layer_count);
        
    int prev_layer;
    int curr_layer;

    f >> prev_layer;

    weight_count = 0;
    output_count = 0;

    m_layer_layout.push_back(prev_layer);
    m_layer_offsets[0] = layer_offset(-1, 0);
        
    for (int i = 1; i < layer_count; i++) {
        f >> curr_layer;

        m_layer_layout.push_back(curr_layer);
                
        output_count += prev_layer + m_bias;

        m_layer_offsets[i] = layer_offset(weight_count, output_count);

        weight_count += (prev_layer + m_bias) * curr_layer;

        prev_layer = curr_layer;
    }

    output_count += curr_layer;

    m_weights.clear();
        
    double w;

    for (int i = 0; i < weight_count; i++) {
        f >> w;

        m_weights.push_back(w);
    }

    m_delta_w.clear();
    m_delta_w.resize(weight_count, 0);

    m_outputs.clear();
    m_outputs.resize(output_count, 1);

    m_errors.clear();
    m_errors.resize(output_count, 0);

    f.close();

    return true;
}

double ann::sigma(double net)
{
    return 1 / (1 + pow(M_E, -net));
}

// output error
double ann::o_error(double t_k, double o_k)
{
    return o_k * (1 - o_k) * (t_k - o_k);
}

// hidden node error
double ann::u_error(double o_h, double sum)
{
    return o_h * (1 - o_h) * sum;
}

// net output error
double ann::e_error(double_vec tk, double_vec ok)
{
    double err = 0;

    for (size_t i = 0; i < tk.size(); i++)
        err += pow(tk[i] - ok[i], 2);

    return err / 2;
}

double ann::delta_w(double error, double x, double prev_w)
{
    return 0.2 * error * x + 0.2 * prev_w;
}
