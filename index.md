# Squidgeefish

{{site.description}}

## Recent Projects

{% assign sorted = site.projects | reverse %}
{% for p in sorted limit:5 %}
<div style="clear:both; margin-bottom:15px">
    <img style="float:left; height:70px; margin-right:10px" src="/assets/{{p.slug}}/thumbnail.jpg"/>
    <div>
        <h3 style="margin-bottom:5px"><a href="{{p.id}}">{{p.title}}</a></h3>
        {{p.description}} <br>
        <i>{{p.date | date : "%B %Y"}}</i>
    </div>
</div>
{% endfor %}


