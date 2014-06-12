from django.conf.urls import patterns, include, url

from range_api.views import EnvList, EnvDetail
from range_api.views import EnvClusterList, EnvClusterDetail
from range_api.views import ClusterClusterList, ClusterClusterDetail
from range_api.views import ClusterOrphansList, ClusterOrphansDetail
from range_api.views import HostOrphansList, HostOrphansDetail
from range_api.views import EnvClusterOrphansList
from range_api.views import OrphansView
from range_api.views import HostsList
from range_api.views import ExpressionView
from range_api.views import GraphCommonAncestor
from range_api.views import GraphBFSParents
from range_api.views import GraphDFSParents
from range_api.views import GraphTopsort
from range_api.views import DotView
from range_api.views import DotTopologyView
from range_api.views import DotEnvView
from range_api.views import DotClusterView
from range_api.views import DotDependencyView
from range_api.views import DotDependencyClusterView
from range_api.views import TopView
from range_api.views import LegacyExpandView
from range_api.views import LegacyListView



urlpatterns = patterns('', 
        #url(r'^range/api/v1/?$', TopView.as_view()),
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
    )

