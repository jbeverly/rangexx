from __future__ import print_function
from librange_python import Range
from rest_framework import views
from rest_framework import status
from rest_framework.request import Request
from rest_framework.response import Response
from serializers import NodeSerializer, ListField
from django.http import HttpResponse

import sys

r = Range('')

CURRENT_VERSION = 2**64 - 1
MAX_DEPTH = 1000

def pretty_node(node, flatten=True):
    pretty = {}
    pretty['dependencies'] = node['dependencies']
    pretty['type'] = node['type']
    pretty['tags'] = node['tags']
    pretty['name'] = node['name']
    if pretty['type'] != 'HOST':
        if flatten:
            pretty['children'] = []

            for cname,child in node['children'].items():
                pretty['children'].append({'type': child['type'], 'name': child['name']})
        else:
            pretty['children'] = node['children']

    return pretty

class TopView(views.APIView):
    def get(self, req, format=None):
        return Response([
            'environments',
            'dot',
            'graph',
            'hosts',
            'orphans',
            'expression',
            ])

################################################################################
################################################################################
class EnvList(views.APIView):
    ############################################################################
    ############################################################################
    def get(self, req, format=None):
        envs = r.all_environments()
        return Response(envs)

    ############################################################################
    ############################################################################
    def post(self, req):
        env = req.DATA
        if isinstance(req.DATA, list):
            if len(req.DATA) > 0:
                env = req.DATA[0]
            else:
                return Response("bad data", status=status.HTTP_400_BAD_REQUEST)


        if(r.create_env(str(env))):
            return Response(pretty_node(r.expand_env(str(env),CURRENT_VERSION, 1)))
        else:
            return Response("environment already exists", status=status.HTTP_409_CONFLICT)


################################################################################
################################################################################
class EnvDetail(views.APIView):
    ############################################################################
    ############################################################################
    def get(self, req, env, format=None):
        print("Getting env %s" % env)
        p = req.QUERY_PARAMS
        depth = str(p.get('depth',1))
        if (depth.lower() == 'full' or depth.lower() == 'all'):
            depth = MAX_DEPTH
        else:
            try:
                depth = int(depth)
            except ValueError:
                depth = 1

        flatten = True
        print(depth)
        if depth > 1:
            flatten=False
        try:
            return Response(pretty_node(r.expand_env(str(env), CURRENT_VERSION, depth),flatten))
        except:
            return Response("environment does not exist", status=status.HTTP_404_NOT_FOUND)

    ############################################################################
    ############################################################################
    def delete(self, req, env, format=None):
        try:
            if(r.remove_env(str(env))):
                return Response(True)
            return Response("error removing", status=status.HTTP_500_INTERNAL_SERVER_ERROR)
        except:
            return Response("environment does not exist", status=status.HTTP_404_NOT_FOUND)

################################################################################
################################################################################
class EnvClusterList(views.APIView):
    ############################################################################
    ############################################################################
    def get(self, req, env, format=None):
        print(req.QUERY_PARAMS)
        if req.QUERY_PARAMS.get('all', False) == 'true':
            try:
                clusters = r.all_clusters(str(env));
            except:
                return Response("no such environment", status=status.HTTP_404_NOT_FOUND)
            return Response(clusters)
        else:
            try:
                children = r.expand_env(str(env))
            except:
                return Response("no such cluster", status=status.HTTP_404_NOT_FOUND)
            childnames = []
            for k,v in children['children'].items():
                if v['type'] == 'CLUSTER':
                    childnames.append(v['name'])
            return Response(childnames)

    ############################################################################
    ############################################################################
    def post(self, req, env):
        cluster = req.DATA
        if isinstance(req.DATA, list):
            if len(req.DATA) > 0:
                cluster = req.DATA[0]
            else:
                return Response("bad data", status=status.HTTP_400_BAD_REQUEST)

        try:
            if r.add_cluster_to_env(str(env), str(cluster)):
                return Response(pretty_node(r.expand_cluster(str(env), str(cluster), CURRENT_VERSION, 1)))
            return Response("error adding", status=status.HTTP_500_INTERNAL_SERVER_ERROR)
        except:
            return Response("cluster already a child of environment", status=status.HTTP_409_CONFLICT)


################################################################################
################################################################################
class ClusterDetail(views.APIView):
    ############################################################################
    ############################################################################
    def get(self, req, env, cluster, parent=None, format=None):
        p = req.QUERY_PARAMS
        depth = str(p.get('depth',1))
        if (depth.lower() == 'full' or depth.lower() == 'all'):
            depth = MAX_DEPTH
        else:
            try:
                depth = int(depth)
            except ValueError:
                depth = 1

        flatten=True
        if depth > 1:
            flatten=False
        try:
            return Response(pretty_node(r.expand_cluster(str(env), str(cluster), CURRENT_VERSION, depth), flatten))
        except:
            return Response("cluster does not exist", status=status.HTTP_404_NOT_FOUND)


################################################################################
# This has to be the ugliest thing I have written in months... 
# ... maybe years... 
################################################################################
class ClusterInfo(object):
    def post(self, req, env, cluster, parent=None):

        if not isinstance(req.DATA, dict):
            return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)
        addhosts = req.DATA.get('addhosts', [])
        keyvalues = req.DATA.get('addkeyvalues', {})
        removekeyvalues = req.DATA.get('removekeyvalues', {})
        removekeys = req.DATA.get('removekeys', [])
        removehosts = req.DATA.get('removehosts', [])
        adddeps = req.DATA.get('adddependencies', [])
        removedeps = req.DATA.get('removedependencies', [])

        if (    not addhosts 
                and not keyvalues and not removekeys 
                and not removehosts and not removekeyvalues
                and not adddeps and not removedeps):

            res = ( "invalid request, must specify one of 'addhosts', "
                    "'removehosts', 'addkeyvalues', 'removekeyvalues', "
                    "'adddependencies', 'removedependencies' ")
            return Response(res, status=status.HTTP_400_BAD_REQUEST)

        if isinstance(addhosts,str) or isinstance(addhosts,unicode):
            addhosts = [addhosts]

        if isinstance(removekeys,str) or isinstance(removekeys,unicode):
            removekeys = [removekeys]

        if isinstance(removehosts,str) or isinstance(removehosts,unicode):
            removehosts = [removehosts]

        if not isinstance(addhosts, list):
            return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

        if not isinstance(keyvalues, dict):
            return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

        for key, value in keyvalues.items():
            if not isinstance(key, str) and not isinstance(key, unicode):
                return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

            if not isinstance(value, list) and not isinstance(value, str) and not isinstance(value, unicode):
                return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

            for v in value:
                if not isinstance(v, str) and not isinstance(v, unicode):
                    return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)


        if not isinstance(removekeyvalues, dict):
            return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

        for key, value in removekeyvalues.items():
            if not isinstance(key, str) and not isinstance(key, unicode):
                return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

            if not isinstance(value, list) and not isinstance(value, str) and not isinstance(value, unicode):
                return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

            for v in value:
                if not isinstance(v, str) and not isinstance(v, unicode):
                    return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)


        if not isinstance(removekeys, list):
            return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

        if not isinstance(removehosts, list):
            return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

        if not isinstance(adddeps, list):
            return Response("invalid request, adddependencies must be a list", status=status.HTTP_400_BAD_REQUEST)

        for dep in adddeps:
            if not isinstance(dep, dict):
                return Response("invalid request, each element of adddependencies must be a dict", status=status.HTTP_400_BAD_REQUEST)

            if len(dep.keys()) != 2:
                return Response("invalid request, each dict in adddependencies must have 2 keys, cluster and env", status=status.HTTP_400_BAD_REQUEST)

            for key, value in dep.items():
                if not isinstance(key, str) and not isinstance(key, unicode):
                    return Response("invalid request, dependency key must be a string", status=status.HTTP_400_BAD_REQUEST)

                if not isinstance(value, str) and not isinstance(value, unicode):
                    return Response("invalid request, dependency value must be a string", status=status.HTTP_400_BAD_REQUEST)

                if str(key).lower() not in ('env', 'cluster'):
                    return Response("invalid request, dependency keys must be one of 'cluser', or 'env'", status=status.HTTP_400_BAD_REQUEST)

        for dep in removedeps:
            if not isinstance(dep, dict):
                return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

            if len(dep.keys()) != 2:
                return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

            for key, value in dep.items():
                if not isinstance(key, str) and not isinstance(key, unicode):
                    return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

                if not isinstance(value, str) and not isinstance(value, unicode):
                    return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)

                if str(key).lower() not in ('env', 'cluster'):
                    return Response("invalid request", status=status.HTTP_400_BAD_REQUEST)



        succeeded = { 'addhosts': [], 'removehosts': [], 'keyvalues': [], 'removekeyvalues': [],
                'removekeys': [], 'adddependencies': [], 'removedependencies': [] }
        failed = { 'addhosts': [], 'removehosts': [], 'keyvalues': [], 'removekeyvalues': [],
                'removekeys': [], 'adddependencies': [], 'removedependencies': [] }
        failure = False
        for host in removehosts:
            try:
                print("removing host %s from %s in %s" % (host, cluster, env))
                if not r.remove_host_from_cluster(str(env), str(cluster), str(host)):
                    failure = True
                    failed['removehosts'].append(host)
                else:
                    succeeded['removehosts'].append(host)
            except RuntimeError:
                failure = True
                failed['removehosts'].append(host)

        for host in addhosts:
            try:
                if not r.add_host_to_cluster(str(env), str(cluster), str(host)):
                    failure = True
                    failed['addhosts'].append(host)
                else:
                    succeeded['addhosts'].append(host)
            except RuntimeError:
                failure = True
                failed['addhosts'].append(host)

        for k,values in keyvalues.items():
            if not isinstance(values, list):
                values = [values]
            v = None
            try:
                for v in values:
                    if not r.add_node_key_value(str(env), str(cluster), str(k), str(v)):
                        failure=True
                        failed['keyvalues'].append( (k, v) )
                    else:
                        succeeded['keyvalues'].append( (k, v) )
            except:
                failed['keyvalues'].append( (k, v) )

        for k,values in removekeyvalues.items():
            if not isinstance(values, list):
                values = [values]
            v = None
            try:
                for v in values:
                    if not r.remove_node_key_value(str(env), str(cluster), str(k), str(v)):
                        failure=True
                        failed['removekeyvalues'].append( (k, v) )
                    else:
                        succeeded['removekeyvalues'].append( (k, v) )
            except:
                failed['removekeyvalues'].append( (k, v) )


        for key in removekeys:
            try:
                if not r.remove_key_from_node(str(env), str(cluster), str(key)):
                    failure=True
                    failed['removekeys'].append(key)
                else:
                    succeeded['removekeys'].append(key)
            except:
                failure=True
                failed['removekeys'].append(key)

        for dep in adddeps:
            try:
                print("Adding dependency from %s#%s to %s#%s" % (env, cluster, dep['env'], dep['cluster']))
                if not r.add_node_ext_dependency(str(env), str(cluster), str(dep['env']), str(dep['cluster'])):
                    failure = True
                    failed['adddependencies'].append(dep)
                else:
                    succeeded['adddependencies'].append(dep)
            except RuntimeError:
                failure = True
                failed['adddependencies'].append(dep)

        for dep in removedeps:
            try:
                if not r.remove_node_ext_dependency(str(env), str(cluster), str(dep['env']), str(dep['cluster'])):
                    failure = True
                    failed['removedependencies'].append(dep)
                else:
                    succeeded['removedependencies'].append(dep)
            except RuntimeError:
                failure = True
                failed['removedependencies'].append(dep)

        if failure:
            return Response({'failed': failed, 'succeeded': succeeded}, status=status.HTTP_409_CONFLICT)
        else:
            return Response(True)

        return





################################################################################
################################################################################
class EnvClusterDetail(ClusterDetail, ClusterInfo):
    ############################################################################
    ############################################################################
    def delete(self, req, env, cluster, format=None):
        try:
            if r.remove_cluster_from_env(str(env), str(cluster)):
                return Response(True)
            return Response("error removing", status=status.HTTP_500_INTERNAL_SERVER_ERROR)
        except:
            return Response("cluster not a child of environment", status=status.HTTP_404_NOT_FOUND)

################################################################################
################################################################################
class ClusterClusterDetail(ClusterDetail, ClusterInfo):
    ############################################################################
    ############################################################################
    def delete(self, req, env, parent, cluster, format=None):
        try:
            if r.remove_cluster_from_cluster(str(env), str(parent), str(cluster)):
                return Response(True)
            return Response("error removing", status=status.HTTP_500_INTERNAL_SERVER_ERROR)
        except:
            return Response("cluster not a child of environment", status=status.HTTP_404_NOT_FOUND)


################################################################################
################################################################################
class ClusterClusterList(views.APIView):
    ############################################################################
    ############################################################################
    def get(self, req, env, cluster, format=None):
        try:
            children = r.expand_cluster(str(env), str(cluster))
        except:
            return Response("no such cluster", status=status.HTTP_404_NOT_FOUND)
        childnames = []
        for k,v in children['children'].items():
            if v['type'] == 'CLUSTER':
                childnames.append(v['name'])
        return Response(childnames)

    ############################################################################
    ############################################################################
    def post(self, req, env, cluster, parent=None):
        parent = cluster
        cluster = req.DATA

        print("parent: %s, cluster %s" % (parent, cluster) )
        if isinstance(req.DATA, list):
            if len(req.DATA) > 0:
                cluster = req.DATA[0]
            else:
                return Response("bad data", status=status.HTTP_400_BAD_REQUEST)
        try:
            if r.add_cluster_to_cluster(str(env), str(parent), str(cluster)):
                return Response(pretty_node(r.expand_cluster(str(env), str(cluster), CURRENT_VERSION, 1)))
            return Response("error adding", status=status.HTTP_500_INTERNAL_SERVER_ERROR)
        except:
            return Response("cluster already a child of environment", status=status.HTTP_409_CONFLICT)


################################################################################
################################################################################
class OrphansView(views.APIView):
    def get(self, format=None):
        return Response(['hosts','clusters']);

################################################################################
################################################################################
class EnvClusterOrphansList(views.APIView):
    def get(self, req, format=None):
        try:
            orphans = r.find_orphaned_nodes()
            envs_with_orphans = {}
            for o in orphans:
                if o[0] == 'CLUSTER':
                    envs_with_orphans[o[1][0:o[1].index('#')]] = True
            return Response(sorted(envs_with_orphans.keys()))
        except RuntimeError as e:
            return Response("error" + e, status=status.HTTP_404_NOT_FOUND)


################################################################################
################################################################################
class ClusterOrphansList(views.APIView):
    def get(self, req, env, format=None):
        try:
            orphans = r.find_orphaned_nodes()
            clusters = []
            for o in orphans:
                if o[0] == 'CLUSTER' and o[1].startswith(str(env) + '#'):
                    clusters.append(o[1][o[1].index('#') + 1:])
            return Response(clusters)

            # too ugly... 
            #return Response([c[1][c[1].index('#')+1:] for c in orphans if c[0] == 'CLUSTER' and c[1].startswith(str(env)+'#')])
        except RuntimeError as e:
            return Response("error" + e, status=status.HTTP_404_NOT_FOUND)

################################################################################
################################################################################
class ClusterOrphansDetail(views.APIView):
    def get(self, req, env, cluster, format=None):
        try:
            return Response(pretty_node(r.expand_cluster(str(env), str(cluster), CURRENT_VERSION, 1)))
        except:
            return Response("no such orphan", status=status.HTTP_404_NOT_FOUND)

    def delete(self, req, env, cluster, format=None):
        try:
            node = r.expand(str(env), str(cluster), CURRENT_VERSION, 1)
        except:
            return Response("node not found", status=status.HTTP_404_NOT_FOUND)

        try:
            if node['type'] == 'CLUSTER':
                if not r.remove_cluster(str(env), str(cluster)):
                    return Response("error removing", status=status.HTTP_500_INTERNAL_SERVER_ERROR)
            else:
                Response("not a cluster", status=status.HTTP_400_BAD_REQUEST)
        except:
            return Response("error removing", status=status.HTTP_404_NOT_FOUND)
        return Response(True)


################################################################################
################################################################################
class HostOrphansList(views.APIView):
    def get(self, req, env, format=None):
        try:
            return Response([c[1] for c in r.find_orphaned_nodes() if c[0] == 'HOST'])
        except:
            return Response("error", status=status.HTTP_404_NOT_FOUND)

################################################################################
################################################################################
class HostOrphansDetail(views.APIView):
    def get(self, req, host, format=None):
        try:
            return Response(pretty_node(r.expand('', str(host), CURRENT_VERSION, depth)))
        except:
            return Response("no such orphan", status=status.HTTP_404_NOT_FOUND)

    def delete(self, req, host, format=None):
        try:
            node = r.expand('', str(host), CURRENT_VERSION, 1)
        except:
            return Response("node not found", status=status.HTTP_404_NOT_FOUND)

        try:
            if node['type'] == 'HOST':
                if not r.remove_host('', str(host)):
                    return Response("error removing", status=status.HTTP_500_INTERNAL_SERVER_ERROR)
            else:
                Response("not a host", status=status.HTTP_400_BAD_REQUEST)
        except:
            return Response("error removing", status=status.HTTP_404_NOT_FOUND)
        return Response(True)


################################################################################
################################################################################
class HostsList(views.APIView):
    ############################################################################
    ############################################################################
    def get(self, req, format=None):
        try:
            return Response(r.all_hosts())
        except:
            return Response("error", status=status.HTTP_404_NOT_FOUND)

    ############################################################################
    ############################################################################
    def post(self, req):
        host = req.DATA
        if isinstance(req.DATA, list):
            if len(req.DATA) > 0:
                host = req.DATA[0]
            else:
                return Response("bad data", status=status.HTTP_400_BAD_REQUEST)
        try:
            if r.add_host(str(host)):
                return Response(pretty_node(r.expand('', str(host), CURRENT_VERSION, 1)))
            return Response("error adding", status=status.HTTP_500_INTERNAL_SERVER_ERROR)
        except:
            return Response("host already exists", status=status.HTTP_409_CONFLICT)


################################################################################
################################################################################
class HostsDetail(views.APIView):
    def get(self, req, host, format=None):
        try:
            return Response(pretty_node(r.expand('', str(host), CURRENT_VERSION, 1)))
        except RuntimeError:
            return Response("host not found", status=status.HTTP_404_NOT_FOUND)


################################################################################
################################################################################
class ExpressionView(views.APIView):
    ############################################################################
    ############################################################################
    def get(self, req, env="", format=None):
        expr = str(req.META.get('QUERY_STRING', ''))
        try:
            return Response(r.expand_range_expression(str(env), expr))
        except RuntimeError:
            return Response("invalid range expression", status=status.HTTP_400_BAD_REQUEST)

    ############################################################################
    ############################################################################
    def post(self, req, env=""):
        expr = str(req.DATA)
        try:
            return Response(r.expand_range_expression(str(env), expr))
        except RuntimeError:
            return Response("invalid range expression", status=status.HTTP_400_BAD_REQUEST)


################################################################################
################################################################################
class GraphView(views.APIView):
    def get(self, req, format=None):
        return Response(['nearest_common_ancestor','bfs_search_parents_for_first_key', 'dfs_search_parents_for_first_key', 'environment_topological_sort'])

        
################################################################################
################################################################################
class GraphCommonAncestor(views.APIView):
    def get(self, req, format=None):
        pass

    def post(self, req, format=None):
        pass
        
################################################################################
################################################################################
class GraphBFSParents(views.APIView):
    def get(self, req, format=None):
        pass

    def post(self, req, format=None):
        pass

################################################################################
################################################################################
class GraphDFSParents(views.APIView):
    def get(self, req, format=None):
        pass

    def post(self, req, format=None):
        pass

        
################################################################################
################################################################################
class GraphTopsort(views.APIView):
    def get(self, req, format=None):
        pass

    def post(self, req, format=None):
        pass

################################################################################
################################################################################
class DotView(views.APIView):
    def get(self, req, format=None):
        return Response(['topology','dependency'])

################################################################################
################################################################################
class DotAllEnvView(views.APIView):
    def get(self, req, format=None):
        envs = r.all_environments()
        return Response(envs)

################################################################################
################################################################################
class DotTopologyView(DotAllEnvView):
    pass

################################################################################
################################################################################
class DotDependencyView(DotAllEnvView):
    pass


import pydot
################################################################################
################################################################################
class DotRender(object):
    def __init__(self, name):
        self._name = name
        self.graph = pydot.Dot(self._name, graph_type='digraph')
        self.graph.set_prog('dot')
        self.graph.set_simplify("true")
        return

    def gendfs_(self, node):
        n = pydot.Node(node['name'])
        for cname, child in node['children'].items():
            c = pydot.Node(child['name'])
            c.set_shape('record')
            c.set_style('filled')
            if child['type'] != 'HOST' and child['type'] != 'STRING':
                c.set_fillcolor('#AAAAff')
                self.gendfs_(child)

            self.graph.add_node(c)
            self.graph.add_edge(pydot.Edge(n, c))

    def gendfs(self, node):
        n = pydot.Node(node['name'])
        n.set_shape('record')
        n.set_style('filled')
        n.set_fillcolor('#AAFFAA')
        self.graph.add_node(n)
        self.gendfs_(node)

        return self.graph

    def genclusterdfs_(self, node, count=0):
        if len(node['children']) > 0 or node['type'] == 'CLUSTER':
            sub = pydot.Subgraph(graph_name='cluster_' + node['name'])
            sub.set_label(node['name'])

            nodenum = 0
            for cname, child in node['children'].items():
                childnode = self.genclusterdfs_(child, nodenum)
                nodenum += 1
                if isinstance(childnode,pydot.Subgraph):
                    sub.add_subgraph(childnode)
                else:
                    sub.add_node(childnode)
        else:
            sub = pydot.Node(node['name'])
            sub.set_shape('record')
            if(count % 2 != 0):
                sub.set_pos('0,0!')
        return sub


    def genclusterdfs(self, node):
        childnode = self.genclusterdfs_(node)
        if isinstance(childnode,pydot.Subgraph):
            self.graph.add_subgraph(childnode)
        else:
            self.graph.add_node(childnode)
        self.graph.set_rankdir('TB')
        return

    def dot(self):
        return self.graph.to_string()

    def render(self):
        return self.graph.create_svg()

################################################################################
################################################################################
class DotEnvView(views.APIView):
    def get(self, req, env, format=None):
        env = str(env)
        try:
            g = r.expand_env(env)
        except:
            return Response("environment not found: %s" % env, status=status.HTTP_404_NOT_FOUND)

        rend = DotRender(env)
        if str(req.QUERY_PARAMS.get('cluster',False)) == 'true':
            rend.genclusterdfs(g)
        else:
            rend.gendfs(g)

        if str(req.QUERY_PARAMS.get('dot', False)) == 'true':
            return HttpResponse(rend.dot(), content_type='text/plain')

        return HttpResponse(rend.render(), content_type='image/svg+xml')

################################################################################
################################################################################
class DotClusterView(views.APIView):
    def get(self, req, env, cluster, format=None):
        env = str(env)
        cluster = str(cluster)
        try:
            g = r.expand_cluster(env, cluster)
        except:
            return Response("host not found", status=status.HTTP_404_NOT_FOUND)

        rend = DotRender(env)
        if str(req.QUERY_PARAMS.get('cluster',False)) == 'true':
            rend.getclusterdfs(g)
        else:
            rend.gendfs(g)

        if str(req.QUERY_PARAMS.get('dot', False)) == 'true':
            return HttpResponse(rend.dot(), content_type='text/plain')
        return HttpResponse(rend.render(), content_type='image/svg+xml')


################################################################################
################################################################################
class DotDependencyView(views.APIView):
    def get(self, req, env, format=None):
        pass

################################################################################
################################################################################
class DotDependencyClusterView(views.APIView):
    def get(self, req, env, cluster, format=None):
        pass


################################################################################
################################################################################
class LegacyListView(views.APIView):
    def get(self, req, format=None):
        expr = str(req.META.get('QUERY_STRING', ''))
        res = None
        try:
            res = r.expand_range_expression(str(env), expr)
        except RuntimeError:
            return HttpResponse('', content_type='text/plain')

        res_text = ""
        for item in res:
            res_text += item + '\n'

        return HttpResponse(res_text, content_type='text/plain')

################################################################################
################################################################################
class LegacyExpandView(views.APIView):
    def get(self, req, format=None):
        expr = str(req.META.get('QUERY_STRING', ''))
        res = None
        try:
            res = r.expand_range_expression(str(env), expr)
        except RuntimeError:
            return HttpResponse('', content_type='text/plain')

        res_text = ""
        first = True
        for item in res:
            if not fist:
                res_text += ','
            res_text += item 

        return HttpResponse(res_text, content_type='text/plain')






















