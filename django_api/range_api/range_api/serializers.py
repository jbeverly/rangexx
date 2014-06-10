from rest_framework import serializers

################################################################################
################################################################################
class Node(object):
    ############################################################################
    ############################################################################
    def __init__(self, name, type, children, dependencies, tags):
        self.name = name
        self.type = type
        self.children = children
        self.dependencies = dependencies
        self.tags = tags
        return

################################################################################
################################################################################
class ListField(serializers.Serializer):
    foo = serializers.CharField(max_length=1024)

################################################################################
################################################################################
class TagsField(serializers.WritableField):
#class TagsField(serializers.Serializer):
    def from_native(self, data):
        return data
    def to_native(self, obj):
        return obj
    #name = serializers.CharField(max_length=1024)
    #value = ListField(many=True)
  
################################################################################
################################################################################
class ChildField(serializers.WritableField):
    #nodeid = serializers.CharField(max_length=1024)

    ############################################################################
    ############################################################################
    def from_native(self, data):
        if not isinstance(data, dict): 
            msg = self.error_messages['invalid']
            raise RuntimeError(msg)
        for k,v in data.items():
            if not isinstance(k, str) or not isinstance(v, dict):
                msg = self.error_messages['invalid']
                raise RuntimeError(msg)
        return data

    ############################################################################
    ############################################################################
    def to_native(self, obj):
        return obj
        children = NodeSerializer(data=obj)
        print children.data
        if children.is_valid():
            return children.data
        msg = self.error_messages['invalid']
        raise RuntimeError(msg)


################################################################################
################################################################################
class NodeSerializer(serializers.Serializer):
    name = serializers.CharField(max_length=1024)
    type = serializers.ChoiceField(['HOST','CLUSTER','STRING','ENVIRONMENT'])
    children = ChildField()
    dependencies = ListField(many=True)
    tags = TagsField()

    def restore_object(self, attrs, inst=None):
        if inst is not None:
            inst.name = attrs.get("name", inst.name)
            inst.type = attrs.get("type", inst.type)
            inst.children = attrs.get("children", inst.children)
            inst.dependencies = attrs.get("dependencies", inst.dependencies)
            inst.tags = attrs.get("tags", inst.tags)
            return inst
        return Node(**attrs)


            

#ChildField.base_fields['children'] = NodeSerializer()

