using Microsoft.OpenApi.Models;

var builder = WebApplication.CreateBuilder(args);
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen(c =>
{
    c.SwaggerDoc("v1", new OpenApiInfo { Title = "WEB BROWSER SERVER", Version = "v1" });
});

if (builder.Environment.IsDevelopment())
{
    builder.WebHost.ConfigureKestrel(c =>
    {
        c.ListenLocalhost(3000);
    });
}

var app = builder.Build();

if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI(c =>
    {
        c.SwaggerEndpoint("swagger/v1/swagger.json", "WEB BROWSER SERVER V1");
    });
}

app.MapGet("/", () => "Hello World!");

app.Run();
