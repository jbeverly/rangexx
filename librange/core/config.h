/* * This file is part of range++.
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
#ifndef _RANGE_CORE_CLIENT_CONFIG_H
#define _RANGE_CORE_CLIENT_CONFIG_H

#include "../graph/node_factory.h"
#include "../graph/graphdb_factory.h"
#include "../compiler/compiler_types.h"
#include "../db/config_interface.h"

namespace range {
    //##############################################################################
    //##############################################################################
struct Config {
    public:
        virtual ~Config() = default;

        virtual boost::shared_ptr<db::BackendInterface> db_backend() const
        { return db_backend_; }

        virtual boost::shared_ptr<graph::NodeIfaceAbstractFactory> node_factory() const
        { return node_factory_; }

        virtual boost::shared_ptr<graph::GraphdbAbstractFactory> graph_factory() const
        { return graph_factory_; }

        virtual boost::shared_ptr<compiler::functor_map_t> range_symbol_table() const
        { return range_symbol_table_; }

        virtual bool use_stored() const
        { return use_stored_; }

        virtual std::string stored_mq_name() const
        { return stored_mq_name_; }

        virtual uint32_t stored_request_timeout() const
        { return stored_request_timeout_; }

        virtual uint32_t reader_ack_timeout() const
        { return reader_ack_timeout_; }

    protected:
        Config() = default;

        Config(
                boost::shared_ptr<db::BackendInterface> db_backend,
                boost::shared_ptr<graph::NodeIfaceAbstractFactory> node_factory,
                boost::shared_ptr<graph::GraphdbAbstractFactory> graph_factory,
                boost::shared_ptr<compiler::functor_map_t> range_symbol_table,
                bool use_stored,
                std::string stored_mq_name,
                uint32_t stored_request_timeout,
                uint32_t reader_ack_timeout
              ) 
            :   db_backend_(db_backend),
                node_factory_(node_factory),
                graph_factory_(graph_factory),
                range_symbol_table_(range_symbol_table),
                use_stored_(use_stored),
                stored_mq_name_(stored_mq_name),
                stored_request_timeout_(stored_request_timeout),
                reader_ack_timeout_(reader_ack_timeout)
        { }


        virtual void db_backend(boost::shared_ptr<db::BackendInterface> v)
        { db_backend_ = v; }

        virtual void node_factory(boost::shared_ptr<graph::NodeIfaceAbstractFactory> v)
        { node_factory_ = v; }

        virtual void graph_factory(boost::shared_ptr<graph::GraphdbAbstractFactory> v)
        { graph_factory_ = v; }

        virtual void range_symbol_table(boost::shared_ptr<compiler::functor_map_t> v)
        { range_symbol_table_ = v; }

        virtual void use_stored(bool v)
        { use_stored_ = v; }

        virtual void stored_mq_name(std::string v)
        { stored_mq_name_ = v; }

        virtual void stored_request_timeout(uint32_t v)
        { stored_request_timeout_ = v; }

        virtual void reader_ack_timeout(uint32_t v)
        { reader_ack_timeout_ = v; } 

    private:
        friend boost::shared_ptr<Config> config_builder(const std::string&);

        boost::shared_ptr<db::BackendInterface> db_backend_;
        boost::shared_ptr<graph::NodeIfaceAbstractFactory> node_factory_;
        boost::shared_ptr<graph::GraphdbAbstractFactory> graph_factory_;
        boost::shared_ptr<compiler::functor_map_t> range_symbol_table_;
        bool use_stored_;
        std::string stored_mq_name_;
        uint32_t stored_request_timeout_;
        uint32_t reader_ack_timeout_;


};

} /* namespace range */

#endif
