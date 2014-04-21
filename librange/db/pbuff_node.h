/*
 * This file is part of range++.
 *
 * range++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * range++ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with range++.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _RANGE_DB_PBUFF_NODE_H
#define _RANGE_DB_PBUFF_NODE_H

#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "../graph/node_interface.h"

#include "nodeinfo.pb.h"
#include "db_interface.h"

namespace range {
namespace db { 

//##############################################################################
//##############################################################################
class ProtobufNode : public graph::NodeIface, public boost::enable_shared_from_this<ProtobufNode> {
    public:
    //##########################################################################
    typedef graph::NodeIface::node_t node_t;
    typedef boost::shared_ptr<GraphInstanceInterface> instance_t;
    
        //######################################################################
        inline ProtobufNode()
            : name_(0), instance_(0), wanted_version_(-1), type_(node_type::UNKNOWN), 
                info_initialized(false), info()
        {
        }

        inline ProtobufNode(const std::string& name, instance_t instance, 
                            uint64_t version = -1)
            : name_(name), instance_(instance), wanted_version_(version),
                type_(node_type::UNKNOWN), info_initialized(false)
        {
        }

        //######################################################################
        virtual std::vector<node_t> forward_edges() const override;
        virtual std::vector<node_t> reverse_edges() const override;

        //######################################################################
        virtual std::string name() const override;
        virtual node_type type() const override;
        virtual uint64_t version() const override;
        virtual uint64_t get_wanted_version() const override;
        virtual uint32_t crc32() const override;
        virtual bool is_valid() const override;
        virtual std::unordered_map<std::string, std::vector<std::string>> tags() const override;

        //######################################################################
        //######################################################################
        virtual bool add_forward_edge(
                node_t other, bool update_other_reverse_edge = true) override;

        virtual bool add_reverse_edge(
                node_t other, bool update_other_forward_edge = false) override;

        virtual bool remove_forward_edge(
                node_t other, bool update_other_reverse_edge = true) override;

        virtual bool remove_reverse_edge(
                node_t other, bool update_other_forward_edge = false) override;

        virtual bool update_tag(
                const std::string& key, const std::vector<std::string>& values) override;

        virtual bool delete_tag(const std::string& key) override;

        virtual bool set_wanted_version(uint64_t version) override;

        virtual node_type set_type(node_type type) override;

        virtual bool commit() override;

        virtual void add_graph_version(uint64_t version) override;
        virtual std::vector<uint64_t> graph_versions() const override;

        //######################################################################
        instance_t get_instance() const;
        instance_t get_graph() const;

        instance_t set_instance(instance_t instance);
        instance_t set_graph(instance_t instance);
        
        //######################################################################
        virtual void shutdown() override {
            s_shutdown();
        }

        static inline void s_shutdown() {
            google::protobuf::ShutdownProtobufLibrary();
        }

    //##########################################################################
    //##########################################################################
    private:
        std::string name_;
        instance_t instance_;

        uint64_t wanted_version_;
        static const auto rectype = GraphInstanceInterface::record_type::NODE;

        mutable node_type type_;
        mutable bool info_initialized;
        mutable NodeInfo info;

        //######################################################################
        inline void init_info() const;
        inline GraphInstanceInterface::lock_t info_lock(bool writable = false);
        inline std::vector<node_t> get_edges(const NodeInfo_Edges& edges) const;

};




} // namespace db
} // namespace range

#endif
