# What is Range++ ?

Range++ is a distributed fault-tolerant high-performance graph-database and directory service for storing and retrieving "Cloud" configuration metadata. It is inspired by Yahoo!'s [libcrange (now maintained by square)](https://github.com/square/libcrange) and loosely inspired by Google's ["Chubby" Distribute Lock Service for Loosely-Coupled Distributed Systems](http://research.google.com/archive/chubby.html). 

## How is Range++ like libcrange?
Range++ has a compatible expression syntax, and supports libcrange's HTTP "API" (both list, and range modes), and so is compatible with all Yahoo SECO modules, Salt, and other tools which support libcrange.

## How is Range++ like Chubby?
Range++ implements PAXOS atomic broadcast for guaranteed ordering, and fault tolerance; meaning it is suitable for use in leader-election for clusters, and **non-service-impacting** distributed locking. (Do **NOT** use range++, or any distributed lock service, in any way that could direction or indirectly impact serving)

## How is Range++ Different from libcrange?
In addition to providing API compatibility with libcrange, Range++ also provides a full semantic REST API, convenient graph operations, has a native atomic-write API with full version history retention, and the ability to query that history directly. Also, as Range++'s graph structure has both in- and out-edges, queries that were very expensive in libcrange (clusters, for example) are trivial in range++. With a large number of clusters (> 50,000), the difference in performance is staggering (minutes vs milliseconds)

## How is Range++ Different from chubby?
Range++ is more than a key-value store; it is a fully featured graph database in addition to key-value store, with multiple values per key, per vertex. Range++ also offers a full expression syntax for easier retrieval and use of the information it contains. And because Range++ is a graph database, the somewhat inflexible, and often difficult to scale concept of leader-election can easily be extended from simply "leader of a cluster" to "leader for this shard/partition number", for DHTs with multiple replicas per shard, allowing for more robust and scalable architectures easily.

## What Can I do with range++?
Many things. Range is a collection of (possibly interrelated) graph structures. For example, there is a topological "primary" graph, which represents your environments, clusters, clusters-of-clusters, and hosts within clusters. There is a "dependency" graph, which shows how a cluster may depend on an environment, or a cluster within another environment. etc. You can add your own graph structures as well, allowing you to represent adjacency information for any usecase you may have. For ever vertex in any graph, you can store "tags", which are keys with multiple values. This allows you to persist information about any environment, cluster, host, or other thing. The information you persist could be, for example, what chef or puppet role (or their equivalent in salt, ansible, metahelper, and if your truly unfortunate, cfengine, etc) every host in a cluster should have; effectively turning range into an ENC. You can also use Hiera with salt or puppet, "automatic attributes" in chef backed by range++ to serve parameters/properties for recipes/catalogs, etc. 

Range++, as currently implemented, is designed around the concept of an "environment" being a full-stack deployment. Environments contain clusters. A cluster is a logical association of things. So a cluster could be, for example "frontend", which could contain "webservers", or "backend" which could contain "datastore". Clusters can contain other clusters, or they can contain hosts (or any arbitrary terminal). Clusters may be nested to any depth, and need not necessarily be acyclic (however, usecases where cycles are desired are unlikely.) The idea being, you describe what your full stack looks like when deployed, with "environment" at the top, and clusters-of-clusters-of-<...>-of-clusters. Once you have this description, you can dynamically plug hosts into the clusters to classify their serving role. Range allows you to easily visualize these topologies as graphviz dot SVG (or PNG) images, so you can see what hosts are in which roles, and where they sit within the architecture.

Range also allows you to specify dependency relationships between clusters (and between environments), so you can, for example, encode that your "frontend -> webservers" depend on your "backend -> datastore". This allow allows you to more easily automate deployments by taking advantage of Range++'s topological sort, and similar methods. This also allows you to easily augment your monitoring to avoid "alert-cascade", and provide dashboards that visually display where a fault has occurred, and what that fault is impacting. Range++ dynamically generates SVG and DOT with the dependency graph, which you can then "colorize" in your dashboard (e.g. green for healthy, red for failed) 

Range++ is best when it is being used as a source-of-truth for configuration information about your environment. To be a source of truth, by definition, the information in range++ should be considered "prescriptive". i.e. When information is updated in range++, automation should be notified (via amqp, or similar mechanism) to take action to realize and enforce that prescribed by range.

When used in this way, you can take a snapshot of an "environment", minus the hosts it contains, and copy its state to another environment, thereby "promoting" the configuration state of one environment to another. If done incrementally, and with validation, this would provide a mechanism to orchestrate and automate full, reproducible lifecycle management deployments safely, with rollback, change history, and the ability to audit after the fact.

# Example Range Expression Syntax:

### Simple ranges:
  * node1,node2,node3,node4 == node1..node4 == node1..4
  * node1000..1099 == node1000..99 # auto pads digits to the end of the range
  * 1..100   # numeric only ranges
  * foo1-2.domain.com == foo1.domain.com-foo2.domain.com # domain support
      
      
### Clusters:
A cluster is a way to store node membership into groups. 
    
  * %cluster101 == cluster101
  * %cluster101:ALL or %cluster101:FOO == nodes defined in a specific key of cluster101
  * %%all == assuming %all is a list of clusters, the additional % will expand the list of clusters to node list that is their membership
  * *node == returns the cluster(s) that node is a member of
      
### Operatons:
  * range1,range2  == union
  * range1,-range2 == set difference
  * range1,&range2 == intersection
  * ^range1 == admins for the nodes in range1
  *  range1,-(range2,range3) == () can be used for grouping
  * range1,&/regex/ # all nodes in range1 that match regex
  * range1,-/regex/ # all nodes in range1 that do not match regex
      
### Advanced ranges:
    
   * foo{1,3,5} == foo1,foo3,foo5
   * %cluster30{1,3} == %cluster301,%cluster303
   * %cluster301-7 == nodes in clusters cluster301 to cluster307
   * %all:KEYS == all defined sections in cluster all
   * %{%all} == expands all clusters in %all
   * %all:dc1,-({f,k}s301-7) == names for clusters in dc1 except ks301-7,fs301-7
   * %all:dc1,-|ks| == clusters in dc1, except those matching ks
      
### Functions:

modules can define certain functions to look up data about hosts or clusters
The following are builtin:
    
  * has(KEY;value) - looks for a cluster that has a key with some certain value
  * expand_hosts(cluster) - expand from cluster down, returning all "hosts" at any depth beneath it.
  * expand(cluster) - expand to a JSON structure of the cluster down, with all descendants and their attribtues nested within.
  * all_clusters() - expands to al ist of all clusters
  * * and clusters(node)  - returns all clusters a node is a direct descendant of
  * has(ENVIRONMENT; production) would return any clusters with the a key called ENVIRONMENT set to production
  * mem(CLUSTER; foo.example.com) => which keys under CLUSTER is foo.example.com a member of

# Range REST API
## See the Swagger generated docs.
Below are the URL matchers:

```
url(r'^range/api/v1/environments/$', EnvList.as_view()),
url(r'^range/api/v1/environments/(?P<env>[a-zA-Z0-9\_]+)$', EnvDetail.as_view()),
url(r'^range/api/v1/environments/(?P<env>[a-zA-Z0-9\_]+)/$', EnvClusterList.as_view()),
url(r'^range/api/v1/environments/(?P<env>[a-zA-Z0-9\_]+)/(?P<cluster>[a-zA-Z0-9\_]+)$', EnvClusterDetail.as_view()),
url(r'^range/api/v1/environments/(?P<env>[a-zA-Z0-9\_]+)/(?P<cluster>[a-zA-Z0-9\_]+)/$', ClusterClusterList.as_view()),
url(r'^range/api/v1/environments/(?P<env>[a-zA-Z0-9\_]+)/(?:[a-zA-Z0-9\_]+/)+?(?P<cluster>[a-zA-Z0-9\_]+)/$', ClusterClusterList.as_view()),
url(r'^range/api/v1/environments/(?P<env>[a-zA-Z0-9\_]+)/(?:(?:[a-zA-Z0-9\_]+)/)*(?P<parent>[a-zA-Z0-9\_]+)/(?P<cluster>[a-zA-Z0-9\_]+)$', ClusterClusterDetail.as_view()),


url(r'^range/api/v1/hosts/?$', HostsList.as_view()),

url(r'^range/api/v1/dot/?$', DotView.as_view()),
url(r'^range/api/v1/dot/topology/?$', DotTopologyView.as_view()),
url(r'^range/api/v1/dot/topology/(?P<env>[a-zA-Z0-9\_]+)$', DotEnvView.as_view()),
url(r'^range/api/v1/dot/topology/(?P<env>[a-zA-Z0-9\_]+)/(?P<cluster>[a-zA-Z0-9\_]+)$', DotClusterView.as_view()),

url(r'^range/api/v1/dot/dependency/?$', DotDependencyView.as_view()),
url(r'^range/api/v1/dot/dependency/(?P<env>[a-zA-Z0-9\_]+)/(?P<cluster>[a-zA-Z0-9\_]+)$', DotDependencyClusterView.as_view()),

url(r'^range/api/v1/orphans/?$', OrphansView.as_view()),
url(r'^range/api/v1/orphans/clusters/?$', EnvClusterOrphansList.as_view()),
url(r'^range/api/v1/orphans/clusters/(?P<env>[a-zA-Z0-9\_]+)/?$', ClusterOrphansList.as_view()),
url(r'^range/api/v1/orphans/clusters/(?P<env>[a-zA-Z0-9\_]+)/(?P<cluster>[a-zA-Z0-9\_]+)/?$', ClusterOrphansDetail.as_view()),
url(r'^range/api/v1/orphans/hosts/?$', HostOrphansList.as_view()),
url(r'^range/api/v1/orphans/hosts/(?P<host>[a-zA-Z0-9\_]+)/?$', HostOrphansDetail.as_view()),
url(r'^range/api/v1/expression/?$', ExpressionView.as_view()),
url(r'^range/api/v1/expression/(?P<env>[a-zA-Z0-9\.\_]+?)/?$', ExpressionView.as_view()),
url(r'^range/api/v1/graph/nearest_common_ancestor/?$', GraphCommonAncestor.as_view()),
url(r'^range/api/v1/graph/bfs_search_parents_for_first_key/?$', GraphBFSParents.as_view()),
url(r'^range/api/v1/graph/dfs_search_parents_for_first_key/?$', GraphDFSParents.as_view()),
url(r'^range/api/v1/graph/environment_topological_sort/?$', GraphTopsort.as_view()),

url(r'^range/list/?$', LegacyListView.as_view()),
url(r'^range/expand/?$', LegacyExpandView.as_view()),
url(r'^docs/', include('rest_framework_swagger.urls')),
```
